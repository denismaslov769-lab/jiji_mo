namespace GodjoLauncher;

internal static class Program
{
    [STAThread]
    private static void Main()
    {
        using var mutex = new Mutex(true, "GodjoLauncher_SingleInstance", out bool created);
        if (!created)
        {
            MessageBox.Show("Лаунчер уже запущен.", "Godjo Role Play", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }
        System.Text.Encoding.RegisterProvider(System.Text.CodePagesEncodingProvider.Instance);
        ApplicationConfiguration.Initialize();
        Application.Run(new MainForm());
    }
}
