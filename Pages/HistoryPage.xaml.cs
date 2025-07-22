using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace Sign_Language.Pages
{
    public partial class HistoryPage : Page
    {
        private bool _isDragging;
        private Point _dragStartPoint;
        private double _dragStartOffset;

        public class HistoryItem
        {
            public string HistoryNum { get; set; }
            public DateTime Time { get; set; }
            public string Text { get; set; }
        }

        private readonly MainWindow _mainWindow;

        public HistoryPage(Frame frame)
        {
            InitializeComponent();
            _mainWindow = (MainWindow)Application.Current.MainWindow;
            RefreshHistory();
        }

        private void RefreshHistory()
        {
            HistoryDataGrid.ItemsSource = _mainWindow.RequestHistory();
        }

        private void Back_Click(object sender, RoutedEventArgs e)
        {
            if (NavigationService?.CanGoBack == true)
                NavigationService.GoBack();
        }

        private void DeleteButton_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && btn.DataContext is HistoryItem item)
            {
                _mainWindow.DeleteHistory(item.HistoryNum);
                RefreshHistory();
            }
        }

        private void DeleteAll_Click(object sender, RoutedEventArgs e)
        {
            // 한 번 더 확인
            var result = MessageBox.Show(
                "정말 삭제하시겠습니까?",
                "전체삭제 확인",
                MessageBoxButton.YesNo,
                MessageBoxImage.Warning);

            if (result == MessageBoxResult.Yes)
            {
                // YES 눌렀을 때만 삭제 프로토콜 전송
                _mainWindow.DeleteAllHistory();
                RefreshHistory();  // 목록 갱신
            }
            // NO 면 아무 동작도 하지 않음
        }

        private void PlayButton_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && btn.DataContext is HistoryItem item)
            {
                string url = _mainWindow.SendTextAndReceiveVideoUrl(item.Text);
                // MediaElement에 url 할당 후 재생 로직 추가
            }
        }

        private void HistoryDataGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // 빈 구현 또는 필요 로직
        }

        // ── 드래그로만 가로 스크롤하기 위한 이벤트 핸들러 ──
        private void ScrollViewer_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (sender is ScrollViewer sc)
            {
                _isDragging = true;
                _dragStartPoint = e.GetPosition(sc);
                _dragStartOffset = sc.HorizontalOffset;
                sc.CaptureMouse();
            }
        }

        private void ScrollViewer_PreviewMouseMove(object sender, MouseEventArgs e)
        {
            if (_isDragging && sender is ScrollViewer sc)
            {
                var current = e.GetPosition(sc);
                double delta = _dragStartPoint.X - current.X;
                sc.ScrollToHorizontalOffset(_dragStartOffset + delta);
            }
        }

        private void ScrollViewer_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (sender is ScrollViewer sc)
            {
                _isDragging = false;
                sc.ReleaseMouseCapture();
            }
        }
    }
}
