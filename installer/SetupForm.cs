using System;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace GodjoSetup
{
    internal enum SetupMode { Install, Uninstall }

    /// <summary>Окно установщика в стиле лаунчера: тёмный фон, золотые акценты, без системной рамки.</summary>
    internal sealed class SetupForm : Form
    {
        static readonly Color Bg = Color.FromArgb(14, 15, 20), Panel2 = Color.FromArgb(24, 26, 34), Gold = Color.FromArgb(244, 196, 48),
            Gold2 = Color.FromArgb(255, 145, 0), Txt = Color.FromArgb(235, 235, 240), Grey = Color.FromArgb(140, 144, 160), Red = Color.FromArgb(255, 80, 80);

        readonly SetupMode _mode;
        string _dir;
        int _page;              // 0 — параметры, 1 — процесс, 2 — готово, 3 — ошибка
        int _pct;
        string _status = "", _error = "";
        bool _busy;

        readonly TextBox _path = new TextBox();
        readonly CheckBox _desktop = Check("Ярлык на рабочем столе", true), _menu = Check("Ярлык в меню «Пуск»", true),
            _launch = Check("Запустить лаунчер после установки", true), _userData = Check("Удалить настройки и журнал лаунчера", false);
        readonly GoldButton _main = new GoldButton(), _browse = new GoldButton { Secondary = true, Text = "Обзор…" };

        public SetupForm(SetupMode mode, string dir)
        {
            _mode = mode; _dir = dir;
            Text = Installer.AppName + (mode == SetupMode.Install ? " — установка" : " — удаление");
            FormBorderStyle = FormBorderStyle.None; StartPosition = FormStartPosition.CenterScreen;
            ClientSize = new Size(640, 400); BackColor = Bg; DoubleBuffered = true; Font = new Font("Segoe UI", 9.5f);
            try { using (var s = Assembly.GetExecutingAssembly().GetManifestResourceStream("app.ico")) Icon = new Icon(s); } catch { }

            _path.Text = dir; _path.BorderStyle = BorderStyle.FixedSingle; _path.BackColor = Panel2; _path.ForeColor = Txt;
            _path.Font = new Font("Segoe UI", 10f); _path.SetBounds(40, 196, 440, 26);
            _path.TextChanged += (s, e) => { _dir = _path.Text; Invalidate(); };
            _browse.SetBounds(490, 194, 110, 30);
            _browse.Click += (s, e) =>
            {
                using (var d = new FolderBrowserDialog { Description = "Папка для установки лаунчера", SelectedPath = _dir })
                    if (d.ShowDialog(this) == DialogResult.OK)
                        _path.Text = d.SelectedPath.EndsWith("GodjoRP", StringComparison.OrdinalIgnoreCase) ? d.SelectedPath : Path.Combine(d.SelectedPath, "GodjoRP");
            };
            _desktop.SetBounds(40, 250, 300, 24); _menu.SetBounds(40, 276, 300, 24);
            _launch.SetBounds(40, 250, 340, 24); _userData.SetBounds(40, 250, 340, 24);
            _main.SetBounds(450, 340, 150, 40);
            _main.Click += async (s, e) => await OnMain();
            Controls.AddRange(new Control[] { _path, _browse, _desktop, _menu, _launch, _userData, _main });
            Layout0();
        }

        static CheckBox Check(string text, bool on) => new CheckBox { Text = text, Checked = on, ForeColor = Txt, BackColor = Color.Transparent, FlatStyle = FlatStyle.Flat, Cursor = Cursors.Hand };

        void Layout0()
        {
            bool inst = _mode == SetupMode.Install;
            _path.Visible = _browse.Visible = _desktop.Visible = _menu.Visible = inst && _page == 0;
            _userData.Visible = !inst && _page == 0;
            _launch.Visible = inst && _page == 2;
            _main.Visible = _page != 1;
            _main.Text = _page == 0 ? (inst ? "Установить" : "Удалить") : _page == 2 ? (inst ? "Готово" : "Закрыть") : "Закрыть";
            Invalidate();
        }

        async Task OnMain()
        {
            if (_page >= 2)
            {
                if (_page == 2 && _mode == SetupMode.Install && _launch.Checked)
                    try { Process.Start(new ProcessStartInfo(Path.Combine(_dir, Installer.ExeName)) { WorkingDirectory = _dir, UseShellExecute = true }); } catch { }
                Close(); return;
            }
            if (_mode == SetupMode.Install)
            {
                try { _dir = Path.GetFullPath(_dir.Trim()); }
                catch { MessageBox.Show(this, "Некорректный путь установки.", Text); return; }
                long need = Installer.PayloadSize() + 50L * 1024 * 1024;
                if (Installer.FreeSpace(_dir) < need) { MessageBox.Show(this, $"Недостаточно места на диске: нужно ~{need / 1048576} МБ.", Text); return; }
            }
            _page = 1; _busy = true; Layout0();
            var rep = new Progress<Tuple<string, int>>(t => { _status = t.Item1; _pct = t.Item2; Invalidate(); });
            IProgress<Tuple<string, int>> ip = rep;
            try
            {
                bool desk = _desktop.Checked, menu = _menu.Checked, ud = _userData.Checked; string dir = _dir;
                await Task.Run(() =>
                {
                    if (_mode == SetupMode.Install) Installer.Install(dir, desk, menu, (s, p) => ip.Report(Tuple.Create(s, p)));
                    else Installer.Uninstall(dir, ud, (s, p) => ip.Report(Tuple.Create(s, p)));
                });
                _page = 2;
            }
            catch (Exception ex) { _error = ex.Message; _page = 3; }
            _busy = false; Layout0();
        }

        // ---------------------------------------------------------------- отрисовка
        protected override void OnPaint(PaintEventArgs e)
        {
            var g = e.Graphics; g.SmoothingMode = SmoothingMode.AntiAlias; g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.ClearTypeGridFit;
            // фон с градиентом и «ореолом»
            using (var br = new LinearGradientBrush(ClientRectangle, Color.FromArgb(20, 20, 28), Bg, 60f)) g.FillRectangle(br, ClientRectangle);
            using (var path = new GraphicsPath())
            {
                path.AddEllipse(380, -160, 420, 360);
                using (var pb = new PathGradientBrush(path) { CenterColor = Color.FromArgb(60, Gold2), SurroundColors = new[] { Color.FromArgb(0, Gold2) } })
                    g.FillPath(pb, path);
            }
            using (var p = new Pen(Color.FromArgb(50, Gold))) g.DrawRectangle(p, 0, 0, Width - 1, Height - 1);

            // шапка
            using (var f = new Font("Segoe UI Black", 22f, FontStyle.Bold))
            using (var br = new LinearGradientBrush(new Rectangle(40, 30, 300, 40), Gold, Gold2, 0f))
                g.DrawString("GODJO", f, br, 36, 26);
            using (var f = new Font("Segoe UI", 11f, FontStyle.Bold)) TextRenderer.DrawText(g, "ROLE PLAY", f, new Point(160, 40), Txt);
            TextRenderer.DrawText(g, "v" + Installer.Version, Font, new Point(42, 72), Grey);
            // крестик
            TextRenderer.DrawText(g, "✕", new Font("Segoe UI", 12f), new Rectangle(Width - 44, 8, 36, 30), _busy ? Color.FromArgb(70, 70, 80) : Grey, TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter);

            var big = new Font("Segoe UI Semibold", 14f);
            bool inst = _mode == SetupMode.Install;
            switch (_page)
            {
                case 0:
                    TextRenderer.DrawText(g, inst ? "Установка лаунчера" : "Удаление лаунчера", big, new Point(38, 112), Txt);
                    if (inst)
                    {
                        TextRenderer.DrawText(g, "Лаунчер сам найдёт GTA San Andreas, поставит сборку Godjo (машины, текстуры, интерфейс) и подключит к серверу.", Font, new Rectangle(40, 142, 560, 40), Grey, TextFormatFlags.WordBreak);
                        TextRenderer.DrawText(g, "Папка установки", Font, new Point(38, 176), Txt);
                        long size = 0; try { size = Installer.PayloadSize(); } catch { }
                        TextRenderer.DrawText(g, $"Требуется места: {size / 1048576 + 1} МБ   •   WebView2: {(Installer.HasWebView2() ? "установлен" : "будет установлен")}", Font, new Point(38, 310), Grey);
                    }
                    else
                        TextRenderer.DrawText(g, "Будут удалены лаунчер, ярлыки и запись в «Программах». Файлы GTA и сборки внутри папки игры не трогаются — их можно отключить во вкладке «Сборка».", Font, new Rectangle(40, 146, 560, 60), Grey, TextFormatFlags.WordBreak);
                    break;
                case 1:
                    TextRenderer.DrawText(g, inst ? "Устанавливаем…" : "Удаляем…", big, new Point(38, 140), Txt);
                    DrawBar(g, new Rectangle(40, 200, 560, 12), _pct);
                    TextRenderer.DrawText(g, _status, Font, new Rectangle(40, 222, 480, 22), Grey, TextFormatFlags.EndEllipsis);
                    TextRenderer.DrawText(g, _pct + "%", Font, new Rectangle(520, 222, 80, 22), Gold, TextFormatFlags.Right);
                    break;
                case 2:
                    TextRenderer.DrawText(g, inst ? "Установка завершена ✓" : "Лаунчер удалён ✓", big, new Point(38, 140), Gold);
                    TextRenderer.DrawText(g, inst ? "Запустите лаунчер, укажите ник и нажмите «Играть». Увидимся в штате!" : "Спасибо, что играли на Godjo Role Play.", Font, new Rectangle(40, 180, 560, 40), Grey, TextFormatFlags.WordBreak);
                    break;
                case 3:
                    TextRenderer.DrawText(g, "Ошибка", big, new Point(38, 140), Red);
                    TextRenderer.DrawText(g, _error, Font, new Rectangle(40, 180, 560, 120), Grey, TextFormatFlags.WordBreak);
                    break;
            }
            big.Dispose();
        }

        static void DrawBar(Graphics g, Rectangle r, int pct)
        {
            using (var bg = new SolidBrush(Panel2)) Round(g, bg, r, 6);
            var fill = new Rectangle(r.X, r.Y, Math.Max(r.Height, r.Width * Math.Min(100, pct) / 100), r.Height);
            using (var br = new LinearGradientBrush(fill, Gold, Gold2, 0f)) Round(g, br, fill, 6);
        }

        static void Round(Graphics g, Brush b, Rectangle r, int rad)
        {
            using (var p = new GraphicsPath())
            {
                p.AddArc(r.X, r.Y, rad * 2, rad * 2, 180, 90); p.AddArc(r.Right - rad * 2, r.Y, rad * 2, rad * 2, 270, 90);
                p.AddArc(r.Right - rad * 2, r.Bottom - rad * 2, rad * 2, rad * 2, 0, 90); p.AddArc(r.X, r.Bottom - rad * 2, rad * 2, rad * 2, 90, 90);
                p.CloseFigure(); g.FillPath(b, p);
            }
        }

        // перетаскивание окна и кнопка закрытия
        [DllImport("user32.dll")] static extern bool ReleaseCapture();
        [DllImport("user32.dll")] static extern IntPtr SendMessage(IntPtr h, int msg, int w, int l);
        protected override void OnMouseDown(MouseEventArgs e)
        {
            if (new Rectangle(Width - 44, 8, 36, 30).Contains(e.Location)) { if (!_busy) Close(); return; }
            if (e.Button == MouseButtons.Left) { ReleaseCapture(); SendMessage(Handle, 0xA1, 2, 0); }
        }
        protected override void OnFormClosing(FormClosingEventArgs e) { if (_busy) e.Cancel = true; base.OnFormClosing(e); }
    }

    /// <summary>Плоская кнопка с золотым градиентом.</summary>
    internal sealed class GoldButton : Control
    {
        public bool Secondary;
        bool _hover;
        public GoldButton() { SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.UserPaint | ControlStyles.SupportsTransparentBackColor, true); BackColor = Color.Transparent; Cursor = Cursors.Hand; Font = new Font("Segoe UI Semibold", 10.5f); }
        protected override void OnMouseEnter(EventArgs e) { _hover = true; Invalidate(); }
        protected override void OnMouseLeave(EventArgs e) { _hover = false; Invalidate(); }
        protected override void OnPaint(PaintEventArgs e)
        {
            var g = e.Graphics; g.SmoothingMode = SmoothingMode.AntiAlias;
            var r = new Rectangle(0, 0, Width - 1, Height - 1);
            using (var p = new GraphicsPath())
            {
                int d = 10;
                p.AddArc(r.X, r.Y, d, d, 180, 90); p.AddArc(r.Right - d, r.Y, d, d, 270, 90); p.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90); p.AddArc(r.X, r.Bottom - d, d, d, 90, 90); p.CloseFigure();
                if (Secondary)
                {
                    using (var b = new SolidBrush(_hover ? Color.FromArgb(40, 43, 55) : Color.FromArgb(30, 32, 42))) g.FillPath(b, p);
                    using (var pen = new Pen(Color.FromArgb(70, 244, 196, 48))) g.DrawPath(pen, p);
                }
                else
                    using (var b = new LinearGradientBrush(r, _hover ? Color.FromArgb(255, 214, 90) : Color.FromArgb(244, 196, 48), Color.FromArgb(255, 145, 0), 0f)) g.FillPath(b, p);
            }
            TextRenderer.DrawText(g, Text, Font, r, Secondary ? Color.FromArgb(235, 235, 240) : Color.FromArgb(20, 18, 10), TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter);
        }
        protected override void OnTextChanged(EventArgs e) { base.OnTextChanged(e); Invalidate(); }
    }
}
