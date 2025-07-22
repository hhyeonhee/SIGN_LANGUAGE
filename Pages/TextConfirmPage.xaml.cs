using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace Sign_Language.Pages
{
    public partial class TextConfirmPage : Page
    {
        private readonly string _koreanText;
        private readonly string _englishText;
        private readonly Frame _mainFrame;
        private readonly bool _autoYes;
        private bool _isVideoEnded;

        /// <summary>
        /// 한글 텍스트, 영어 번역, 네비게이션용 Frame, 자동 Yes 실행 여부를 받습니다.
        /// </summary>
        public TextConfirmPage(string koreanText, string englishText, Frame mainFrame, bool autoYes = false)
        {
            InitializeComponent();

            _koreanText = koreanText;
            _englishText = englishText;
            _mainFrame = mainFrame;
            _autoYes = autoYes;
            _isVideoEnded = false;

            // UI 세팅
            ResultTextBlock.Text = _koreanText;
            EnglishTextBlock.Text = _englishText;
            VideoLabel.Visibility = Visibility.Collapsed;
            VideoBorder.Visibility = Visibility.Collapsed;
            ReplayButton.Visibility = Visibility.Collapsed;

            // autoYes일 때만 페이지 로드 후 자동 실행
            if (_autoYes)
                this.Loaded += OnPageLoaded;
        }

        // 한 번만 자동으로 YesButton_Click 호출
        private void OnPageLoaded(object sender, RoutedEventArgs e)
        {
            this.Loaded -= OnPageLoaded;
            YesButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
        }

        private void Back_Click(object sender, RoutedEventArgs e)
        {
            if (NavigationService?.CanGoBack == true)
                NavigationService.GoBack();
        }

        private void NoButton_Click(object sender, RoutedEventArgs e)
        {
            if (NavigationService?.CanGoBack == true)
                NavigationService.GoBack();
        }

        private async void YesButton_Click(object sender, RoutedEventArgs e)
        {
            // UI 전환
            ButtonPanel.Visibility = Visibility.Collapsed;
            ConfirmMessage.Visibility = Visibility.Collapsed;
            VideoLabel.Visibility = Visibility.Visible;
            VideoBorder.Visibility = Visibility.Visible;

            try
            {
                var mainWin = (MainWindow)Application.Current.MainWindow;
                string videoUrl = await System.Threading.Tasks.Task.Run(() =>
                    mainWin.SendTextAndReceiveVideoUrl(_koreanText)
                );

                if (!string.IsNullOrEmpty(videoUrl))
                {
                    VideoPlayer.Source = new Uri(videoUrl, UriKind.Absolute);
                    VideoPlayer.Position = TimeSpan.Zero;
                    VideoPlayer.Play();
                    _isVideoEnded = false;
                    ReplayButton.Visibility = Visibility.Collapsed;
                }
                else
                {
                    MessageBox.Show("영상 링크를 받아오지 못했습니다.", "Error");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("영상 재생 중 오류: " + ex.Message, "Error");
            }
        }

        private void VideoPlayer_MediaEnded(object sender, RoutedEventArgs e)
        {
            _isVideoEnded = true;
            VideoPlayer.Stop();
            ReplayButton.Visibility = Visibility.Visible;
        }

        private void VideoPlayer_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (_isVideoEnded)
                RestartVideo();
        }

        private void ReplayButton_Click(object sender, RoutedEventArgs e)
        {
            RestartVideo();
        }

        private void RestartVideo()
        {
            VideoPlayer.Position = TimeSpan.Zero;
            VideoPlayer.Play();
            _isVideoEnded = false;
            ReplayButton.Visibility = Visibility.Collapsed;
        }
    }
}
