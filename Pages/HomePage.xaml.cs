using System.Windows.Controls;

namespace Sign_Language.Pages
{
    public partial class HomePage : Page
    {
        private Frame _mainFrame;
        public HomePage(Frame mainFrame)
        {
            InitializeComponent();
            _mainFrame = mainFrame;
        }

        private void Convert_Click(object s, System.Windows.RoutedEventArgs e)
            => _mainFrame.Navigate(new ConvertPage(_mainFrame));

        private void Learn_Click(object s, System.Windows.RoutedEventArgs e)
            => _mainFrame.Navigate(new LearnPage(_mainFrame));

        private void History_Click(object s, System.Windows.RoutedEventArgs e)
            => _mainFrame.Navigate(new HistoryPage(_mainFrame));
    }
}
