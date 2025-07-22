using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;

namespace Sign_Language.Pages
{
    public partial class LearnPage : Page
    {
        private Frame _mainFrame;

        public LearnPage(Frame mainFrame)
        {
            InitializeComponent();
            _mainFrame = mainFrame;
        }

        private void Back_Click(object sender, RoutedEventArgs e)
        {
            if (NavigationService?.CanGoBack == true)
                NavigationService.GoBack();
        }

        private void CategoryButton_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button button && button.Tag is string categoryName)
            {
                // 버튼의 순서(index)를 기반으로 카테고리 번호 계산
                int index = CategoryGrid.Children.IndexOf(button);
                int categoryNum = index + 1;  // 1~16

                var mainWin = (MainWindow)Application.Current.MainWindow;
                // RequestWordListByCategory는 string 파라미터를 받으므로 ToString() 사용
                List<string> wordList = mainWin.RequestWordListByCategory(categoryNum.ToString());

                var page = new WordListPage(wordList, categoryNum, _mainFrame);
                _mainFrame.Navigate(page);
            }
        }
    }
}
