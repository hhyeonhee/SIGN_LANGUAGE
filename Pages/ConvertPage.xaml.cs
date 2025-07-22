// ConvertPage.xaml.cs
using Microsoft.CognitiveServices.Speech;
using Microsoft.CognitiveServices.Speech.Audio;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Imaging;
using WpfAnimatedGif;

namespace Sign_Language.Pages
{
    public partial class ConvertPage : Page
    {
        // 1) 음성 인식용 Speech 서비스 키/리전 (기존과 동일)
        private readonly string speechKey = "BSYcauBy5WJVOW8JiZaQxteHbqIyfjU897BdKazEjBHg0HJ1oK3IJQQJ99BGACNns7RXJ3w3AAAYACOG10c8";
        private readonly string speechRegion = "koreacentral";

        // 2) DeepL API 키 (https://www.deepl.com/pro#developer 에서 발급)
        private readonly string deepLApiKey = "bf2180e9-3edb-46d6-a12e-f40dc4810af0:fx";
        //    무료(free) 계정용 엔드포인트
        private const string deepLEndpoint = "https://api-free.deepl.com/v2/translate";

        private readonly Frame _mainFrame;

        public ConvertPage(Frame mainFrame)
        {
            InitializeComponent();
            _mainFrame = mainFrame;
            // 로딩 그리드 초기 숨김
            loadingGrid.Visibility = Visibility.Collapsed;
        }

        // ← 버튼 클릭 시 이전 페이지로 돌아가기
        private void Back_Click(object sender, RoutedEventArgs e)
        {
            if (NavigationService?.CanGoBack == true)
                NavigationService.GoBack();
        }

        // 녹음 버튼 클릭 핸들러
        private async void RecordButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ShowLoadingGif();

                // 1) SpeechRecognizer 구성
                var config = SpeechConfig.FromSubscription(speechKey, speechRegion);
                config.SpeechRecognitionLanguage = "ko-KR";

                using var audioInput = AudioConfig.FromDefaultMicrophoneInput();
                using var recognizer = new SpeechRecognizer(config, audioInput);

                // 음성 인식 실행
                var speechResult = await recognizer.RecognizeOnceAsync();
                loadingGrid.Visibility = Visibility.Collapsed;

                if (speechResult.Reason == ResultReason.RecognizedSpeech)
                {
                    string koreanText = speechResult.Text;

                    // 2) DeepL 번역 API 호출
                    string englishText = await TranslateWithDeepLAsync(koreanText);

                    // 3) 결과 페이지로 네비게이트 (자동 Yes 실행)
                    _mainFrame.Navigate(new TextConfirmPage(
                        koreanText,
                        englishText,
                        _mainFrame,
                        false    // 자동 Yes 처리
                    ));
                }
                else
                {
                    MessageBox.Show("음성을 인식하지 못했습니다.", "Error");
                }
            }
            catch (Exception ex)
            {
                loadingGrid.Visibility = Visibility.Collapsed;
                MessageBox.Show($"오류 발생: {ex.Message}", "Error");
            }
        }

        /// <summary>
        /// DeepL API를 호출하여 한국어 문장을 영어로 번역
        /// </summary>
        private async Task<string> TranslateWithDeepLAsync(string inputText)
        {
            using var client = new HttpClient();
            // DeepL 인증 헤더
            client.DefaultRequestHeaders.Add("Authorization", $"DeepL-Auth-Key {deepLApiKey}");

            // 폼 데이터에 텍스트와 언어코드 지정
            var form = new FormUrlEncodedContent(new[]
            {
                new KeyValuePair<string, string>("text",        inputText),
                new KeyValuePair<string, string>("source_lang", "KO"),
                new KeyValuePair<string, string>("target_lang", "EN")
            });

            // DeepL 번역 호출
            var response = await client.PostAsync(deepLEndpoint, form);
            response.EnsureSuccessStatusCode();

            var json = await response.Content.ReadAsStringAsync();
            // 응답 예시: {"translations":[{"detected_source_language":"KO","text":"Hello"}]}
            var obj = JObject.Parse(json);
            var translations = obj["translations"];
            return translations?[0]?["text"]?.ToString() ?? string.Empty;
        }

        
        private void ShowLoadingGif()
        {
            var uri = new Uri("pack://application:,,,/imgs/loading.gif", UriKind.Absolute);
            var img = new BitmapImage(uri);
            ImageBehavior.SetAnimatedSource(GifImage, img);
            loadingGrid.Visibility = Visibility.Visible;
        }
    }
}
