namespace GodjoLauncher;

/// <summary>Окно с прогрессом упаковки игры (--make-game-pack).</summary>
internal sealed class PackProgressForm : Form
{
    private readonly ProgressBar _bar = new() { Dock = DockStyle.Top, Height = 28, Maximum = 1000 };
    private readonly Label _pct = new() { Dock = DockStyle.Top, Height = 40, Font = new Font("Segoe UI", 16f, FontStyle.Bold), TextAlign = ContentAlignment.MiddleCenter, Text = "0 %" };
    private readonly Label _text = new() { Dock = DockStyle.Fill, Font = new Font("Segoe UI", 9.5f), Text = "Подготовка..." };
    private readonly string _gta, _out; private readonly int _mb;
    private bool _done;

    public PackProgressForm(string gta, string outDir, int mb)
    {
        _gta = gta; _out = outDir; _mb = mb;
        Text = "Godjo — упаковка игры";
        Width = 560; Height = 200; StartPosition = FormStartPosition.CenterScreen;
        FormBorderStyle = FormBorderStyle.FixedDialog; MaximizeBox = false;
        Padding = new Padding(14);
        Controls.Add(_text); Controls.Add(new Panel { Dock = DockStyle.Top, Height = 10 }); Controls.Add(_bar); Controls.Add(_pct);
        Shown += async (_, _) => await RunAsync();
        FormClosing += (_, e) =>
        {
            if (!_done && MessageBox.Show(this, "Упаковка ещё идёт. Прервать?", Text, MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes)
                e.Cancel = true;
            else if (!_done) Environment.Exit(1);
        };
    }

    private async Task RunAsync()
    {
        var started = DateTime.Now;
        void Report(double p, string t) => BeginInvoke(() =>
        {
            _bar.Value = Math.Clamp((int)(p * 10), 0, 1000);
            var el = DateTime.Now - started;
            string eta = p > 1 && p < 100 ? $"   ·   осталось ~{TimeSpan.FromSeconds(el.TotalSeconds * (100 - p) / p):mm\\:ss}" : "";
            _pct.Text = $"{p:0} %{eta}";
            _text.Text = t;
            Text = $"Godjo — упаковка игры {p:0}%";
        });
        try
        {
            var res = await Task.Run(() => GameDownloader.MakePack(_gta, _out, _mb, s => Log.Write(s), Report));
            _done = true;
            MessageBox.Show(this, res, "Godjo — сборка игры", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        catch (Exception ex)
        {
            _done = true;
            MessageBox.Show(this, ex.Message, "Godjo — сборка игры", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        Close();
    }
}
