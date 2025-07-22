using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Windows;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Sign_Language.Pages;

namespace Sign_Language
{
    public partial class MainWindow : Window
    {
        private TcpClient client;
        private NetworkStream stream;

        public MainWindow()
        {
            InitializeComponent();
            ConnectToServer();
            SendStartupProtocol();
            MainFrame.Navigate(new HomePage(MainFrame));
        }

        private void ConnectToServer()
        {
            try
            {
                client = new TcpClient("10.10.20.108", 12345);
                stream = client.GetStream();
            }
            catch (Exception ex)
            {
                MessageBox.Show("서버 연결 실패: " + ex.Message);
            }
        }

        private void SendStartupProtocol()
        {
            try
            {
                string ip = GetLocalIPAddress();
                var data = new
                {
                    PROTOCOL = "1",
                    NAME = ip
                };
                SendJson(data);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"초기 메시지 전송 실패: {ex.Message}");
            }
        }

        private string GetLocalIPAddress()
        {
            var host = Dns.GetHostEntry(Dns.GetHostName());
            foreach (var ip in host.AddressList)
            {
                if (ip.AddressFamily == AddressFamily.InterNetwork)
                    return ip.ToString();
            }
            return "Unknown";
        }

        private void SendJson(object obj)
        {
            string json = JsonConvert.SerializeObject(obj);
            byte[] data = Encoding.UTF8.GetBytes(json);
            stream.Write(data, 0, data.Length);
        }

        private (string, string) ReceiveJsonPair()
        {
            byte[] buffer = new byte[8192];
            using (var ms = new MemoryStream())
            {
                while (true)
                {
                    int bytesRead = stream.Read(buffer, 0, buffer.Length);
                    if (bytesRead == 0) break;
                    ms.Write(buffer, 0, bytesRead);

                    string fullText = Encoding.UTF8.GetString(ms.ToArray());
                    int firstJsonEnd = fullText.IndexOf("}") + 1;
                    if (firstJsonEnd <= 0 || firstJsonEnd >= fullText.Length)
                        continue;

                    string json1 = fullText.Substring(0, firstJsonEnd);
                    string json2 = fullText.Substring(firstJsonEnd);

                    try
                    {
                        JsonConvert.DeserializeObject<Dictionary<string, object>>(json1);
                        JsonConvert.DeserializeObject<Dictionary<string, object>>(json2);
                        return (json1, json2);
                    }
                    catch
                    {
                        continue;
                    }
                }
            }
            throw new Exception("두 JSON을 정상적으로 수신하지 못했습니다.");
        }

        // 단일 JSON 수신 헬퍼 (비디오 URL 등)
        private string ReceiveJson()
        {
            byte[] buffer = new byte[8192];
            int bytesRead = stream.Read(buffer, 0, buffer.Length);
            if (bytesRead <= 0)
                throw new Exception("서버 응답이 없습니다.");
            return Encoding.UTF8.GetString(buffer, 0, bytesRead);
        }

        // 히스토리 요청 (PROTOCOL 6)
        public List<HistoryPage.HistoryItem> RequestHistory()
        {
            var request = new Dictionary<string, object>
            {
                { "PROTOCOL", "6" }
            };
            SendJson(request);

            var (lengthJson, contentJson) = ReceiveJsonPair();
            var contentObj = JObject.Parse(contentJson);
            var dataArray = contentObj["DATA"] as JArray;

            var historyList = new List<HistoryPage.HistoryItem>();
            if (dataArray != null)
            {
                foreach (var elem in dataArray)
                {
                    historyList.Add(new HistoryPage.HistoryItem
                    {
                        HistoryNum = elem["HISTORY_NUM"]?.ToString(),
                        Time = DateTime.Parse(elem["TIME"]?.ToString()),
                        Text = elem["TEXT"]?.ToString()
                    });
                }
            }
            return historyList;
        }

        // 카테고리별 단어 리스트 요청 (PROTOCOL 4)
        public List<string> RequestWordListByCategory(string category)
        {
            var request = new Dictionary<string, object>
    {
        { "PROTOCOL", "4" },
        { "CATEGORY_NUM", category }
    };
            SendJson(request);

            // 두 개의 JSON(헤더/본문) 수신
            var (lengthJson, contentJson) = ReceiveJsonPair();
            var contentObj = JObject.Parse(contentJson);

            // CONTENT 배열 읽기
            var contentArray = contentObj["CONTENT"] as JArray;
            var wordList = new List<string>();

            if (contentArray != null)
            {
                foreach (var token in contentArray)
                {
                    // 서버가 "\"가느다랗다\"" 같은 형태로 보내면
                    // 외부 따옴표를 제거해 순수 단어만 취득
                    string word = token.ToString().Trim('"');
                    wordList.Add(word);
                }
            }
            else
            {
                // 디버그용: 반환된 JSON 전체를 출력해 봅니다
                MessageBox.Show("서버 응답 CONTENT: " + contentJson);
            }

            return wordList;
        }


        // 텍스트 전송 후 비디오 URL 수신 (PROTOCOL 2)
        public string SendTextAndReceiveVideoUrl(string text)
        {
            var request = new Dictionary<string, object>
            {
                { "PROTOCOL", "2" },
                { "TEXT", text }
            };
            SendJson(request);

            // 단일 JSON 응답에서 VIDEO_PATH 파싱
            string json = ReceiveJson();
            var obj = JObject.Parse(json);
            if (obj.TryGetValue("VIDEO_PATH", out JToken pathToken))
                return pathToken.ToString();

            MessageBox.Show("서버에서 VIDEO_PATH를 받지 못했습니다.");
            return null;
        }

        /// <summary>
        /// 단일 히스토리 삭제 요청 (PROTOCOL 7)
        /// </summary>
        public void DeleteHistory(string historyNum)
        {
            var request = new Dictionary<string, object>
            {
                { "PROTOCOL", "7" },
                { "HISTORY_NUM", historyNum }
            };
            SendJson(request);
            // 필요 시 서버 응답을 기다리는 로직 추가
        }

        /// <summary>
        /// 전체 히스토리 삭제 요청 (PROTOCOL 8)
        /// </summary>
        public void DeleteAllHistory()
        {
            var request = new Dictionary<string, object>
            {
                { "PROTOCOL", "8" }
            };
            SendJson(request);
        }

        protected override void OnClosed(EventArgs e)
        {
            base.OnClosed(e);
            stream?.Close();
            client?.Close();
        }
    }
}
