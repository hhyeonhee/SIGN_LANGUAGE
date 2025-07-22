using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace Sign_Language.Pages
{
    public partial class WordListPage : Page
    {
        private List<string> allWords;
        private List<string> filteredWords;
        private int itemsPerPage = 8;

        public int CurrentPage = 1;
        public int TotalPages = 1;

        private Frame mainFrame;

        public WordListPage(List<string> wordList, int categoryNum, Frame mainFrame)
        {
            InitializeComponent();
            this.allWords = wordList;
            this.mainFrame = mainFrame;
            FilterWords("");
        }

        private void Back_Click(object sender, RoutedEventArgs e)
        {
            if (NavigationService?.CanGoBack == true)
                NavigationService.GoBack();
        }

        private void SearchBox_TextChanged(object sender, TextChangedEventArgs e)
            => FilterWords(SearchBox.Text);

        private void Search_Click(object sender, RoutedEventArgs e)
            => FilterWords(SearchBox.Text);

        private void FilterWords(string keyword)
        {
            filteredWords = string.IsNullOrWhiteSpace(keyword)
                ? allWords
                : allWords.Where(w => w.Contains(keyword)).ToList();
            CurrentPage = 1;
            DisplayWords();
        }

        private void DisplayWords()
        {
            WordListPanel.Children.Clear();

            TotalPages = (int)Math.Ceiling((double)filteredWords.Count / itemsPerPage);
            var pageItems = filteredWords
                .Skip((CurrentPage - 1) * itemsPerPage)
                .Take(itemsPerPage);

            foreach (var w in pageItems)
                WordListPanel.Children.Add(CreateWordCard(w));

            LoadPaginationButtons();
        }

        // ★ 카드 레이아웃을 Grid로 바꿔서 play 버튼을 우측 끝에 고정
        private Border CreateWordCard(string word)
        {
            // 따옴표 제거
            var cleanText = word.Trim('"');

            // 내부 레이아웃용 Grid
            var grid = new Grid
            {
                Margin = new Thickness(0),
                HorizontalAlignment = HorizontalAlignment.Stretch
            };
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });

            // 단어 텍스트
            var tb = new TextBlock
            {
                Text = cleanText,
                FontSize = 16,
                VerticalAlignment = VerticalAlignment.Center,
                Margin = new Thickness(5, 0, 0, 0)
            };
            Grid.SetColumn(tb, 0);

            // ▶ 재생 버튼: 클릭하면 TextConfirmPage 로 이동
            var btn = new Button
            {
                Width = 30,
                Height = 30,
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Content = new Image
                {
                    Source = new BitmapImage(new Uri("/imgs/play_circle.png", UriKind.Relative)),
                    Width = 20,
                    Height = 20
                }
            };
            btn.Click += (_, __) =>
            {
                // 이 페이지가 생성될 때 전달받은 Frame 인스턴스(mainFrame)로 내비게이트
                mainFrame.Navigate(new TextConfirmPage(cleanText,"", mainFrame, true));
            };
            Grid.SetColumn(btn, 1);

            grid.Children.Add(tb);
            grid.Children.Add(btn);

            // 카드 스타일
            return new Border
            {
                Background = Brushes.White,
                BorderBrush = (Brush)new BrushConverter().ConvertFromString("#DDDDDD"),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(12),
                Margin = new Thickness(0, 5, 0, 5),
                Padding = new Thickness(5),
                Child = grid
            };
        }



        // ★ 페이징을 5개씩 묶어서, 이전/다음 그룹 이동 화살표 추가
        private void LoadPaginationButtons()
        {
            PageButtonPanel.Children.Clear();

            const int groupSize = 5;
            int groupIndex = (CurrentPage - 1) / groupSize;
            int startPage = groupIndex * groupSize + 1;
            int endPage = Math.Min(startPage + groupSize - 1, TotalPages);

            // 이전 그룹
            if (startPage > 1)
            {
                var prevGroup = new Button
                {
                    Content = new Image
                    {
                        Source = new BitmapImage(new Uri("/imgs/arrow_left.png", UriKind.Relative)),
                        Width = 20,
                        Height = 20
                    },
                    Background = Brushes.Transparent,
                    BorderThickness = new Thickness(0),
                    Margin = new Thickness(2)
                };
                prevGroup.Click += (_, __) => {
                    CurrentPage = startPage - 1;
                    DisplayWords();
                };
                PageButtonPanel.Children.Add(prevGroup);
            }

            // 페이지 번호 (startPage ~ endPage)
            for (int i = startPage; i <= endPage; i++)
            {
                var pBtn = new Button
                {
                    Content = i.ToString(),
                    Width = 30,
                    Height = 30,
                    Margin = new Thickness(2),
                    Background = (i == CurrentPage)
                        ? (Brush)new BrushConverter().ConvertFromString("#2196F3")
                        : Brushes.White,
                    Foreground = (i == CurrentPage) ? Brushes.White : Brushes.Black,
                    BorderThickness = new Thickness(0),
                    Tag = i
                };
                pBtn.Click += (s, e) => {
                    CurrentPage = (int)(s as Button).Tag;
                    DisplayWords();
                };
                PageButtonPanel.Children.Add(pBtn);
            }

            // 다음 그룹
            if (endPage < TotalPages)
            {
                var nextGroup = new Button
                {
                    Content = new Image
                    {
                        Source = new BitmapImage(new Uri("/imgs/arrow_right.png", UriKind.Relative)),
                        Width = 20,
                        Height = 20
                    },
                    Background = Brushes.Transparent,
                    BorderThickness = new Thickness(0),
                    Margin = new Thickness(2)
                };
                nextGroup.Click += (_, __) => {
                    CurrentPage = endPage + 1;
                    DisplayWords();
                };
                PageButtonPanel.Children.Add(nextGroup);
            }
        }
    }
}
