using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace wadmake
{
    class Program
    {
        [STAThread]
        static void Main(string[] args)
        {
            MessageBox.Show("This tool is no longer maintained download Obsidian instead.", "Deprecated", MessageBoxButtons.OK, MessageBoxIcon.Information);
            Process.Start("https://github.com/Crauzer/Obsidian");
        }
    }
}
