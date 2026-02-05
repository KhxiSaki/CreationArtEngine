using System;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace CEEditor
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private DispatcherTimer timer;
        private bool isPlaying = false;
        
        public MainWindow()
        {
            InitializeComponent();
            InitializeTimer();
            LogToConsole("Game Editor initialized successfully.");
            LogToConsole("Ready to create amazing games!");
        }
        
        private void InitializeTimer()
        {
            timer = new DispatcherTimer();
            timer.Interval = TimeSpan.FromSeconds(1);
            timer.Tick += Timer_Tick;
            timer.Start();
        }
        
        private void Timer_Tick(object sender, EventArgs e)
        {
            StatusTime.Content = DateTime.Now.ToString("HH:mm:ss");
        }
        
        private void LogToConsole(string message)
        {
            string timestamp = DateTime.Now.ToString("HH:mm:ss");
            ConsoleOutput.AppendText($"LogPlay: {message}\n");
            ConsoleOutput.ScrollToEnd();
        }
        
        private void NewBtn_Click(object sender, RoutedEventArgs e)
        {
            LogToConsole("Created new level");
        }
        
        private void OpenBtn_Click(object sender, RoutedEventArgs e)
        {
            LogToConsole("Opening level dialog");
        }
        
        private void SaveBtn_Click(object sender, RoutedEventArgs e)
        {
            LogToConsole("Level saved successfully");
        }
        
        private void PlayBtn_Click(object sender, RoutedEventArgs e)
        {
            if (!isPlaying)
            {
                isPlaying = true;
                LogToConsole("Started play session");
                LogToConsole("Map loaded");
                PlayBtn.Background = new SolidColorBrush(Color.FromRgb(78, 201, 176)); // UE5 green
            }
        }
        
        private void PauseBtn_Click(object sender, RoutedEventArgs e)
        {
            if (isPlaying)
            {
                LogToConsole("Play session paused");
            }
        }
        
        private void StopBtn_Click(object sender, RoutedEventArgs e)
        {
            if (isPlaying)
            {
                isPlaying = false;
                LogToConsole("Stopped play session");
                LogToConsole("Returned to editor");
                PlayBtn.Background = new SolidColorBrush(Color.FromRgb(78, 201, 176)); // Reset to UE5 green
            }
        }
    }
}