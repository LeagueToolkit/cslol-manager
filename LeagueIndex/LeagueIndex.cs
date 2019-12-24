using Fantome.Libraries.League.IO.WAD;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LeagueIndex
{
    public class LeagueIndex
    {
        Dictionary<uint, WADFile> _lookup;
        static LeagueIndex FromPathOrConfig(string path, string config)
        {


            return new LeagueIndex(path);
        }

        private LeagueIndex(string path)
        {
            foreach(var wad in Direc.EnumerateFiles)
        }

    }
}
