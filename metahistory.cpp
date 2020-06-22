/*****************************************************************************
 h (metahistory) - utility to either store or search command line history
                 - overcomes some shortcomings with the bash history builtin
                 - adds working directory to command line history and search
                 - skips common OS commands and repeated commands

Setup:

   make sure this command 'h' is in your path
   add the following lines to your .bashrc:

      trap 'cmd=$BASH_COMMAND; h -s $cmd' DEBUG
      export MH_STORE=~/.mh_store
      export MH_SKIP=~/.mh_skip

Usage:

   there are two major modes of operation, save or search
   save operation is called after every command by the bash trap shown above

      h -s [cmd]

   search is run by the user to look for previously executed commands
   using an arbitrary substring 

      h [substring]

   common OS (or other) commands may be skipped by adding to a newline
   separated list in file pointed to $METAH_SKIP

   there are 

Syntax Summary:

      save mode for base trap only:

         h -s [cmd]
      
      search mode for user:

         h [-r|-p] [path] [search]

Search Modes:

      show no paths for one match                           h [match]
      show all paths for one match                          h -p match
      show all strings in one path (match all)              h [path]
      show all strings in one path recursively (match all)  h -r [path]
      show match for one path                               h [path] [match]
      show match for one path recursively                   h -r [path] [match]

Examples:

   find commands with the string 'make':
   
      'h make'               
   
   find commands with the string 'make' and dump all working directories:

      'h -p make'                  

   find commands with the string 'make' and dump all working directories
    recursively below the specified path:

      'h -r ~/jbronte make'

   find commands with the string 'make' run from the specified directory:

      'h ~/jbronte make'

   find all commands run from the specified directory:

      'h ~/jbronte/utils/metahistory'


*****************************************************************************/
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <regex>

#define HERE printf("%d\n",__LINE__);

using namespace std;

typedef enum {
   e_null,           
   e_simple,         // show matches no paths
   e_showpaths,      // show matches and paths
   e_onepath,        // show matches for one path
   e_onepath_r,      // show matches for one path recursively
   e_onepath_all,    // show all cmds for one path
   e_onepath_all_r,  // show all cmds for one path recursively
   e_max,  
} search_e;

typedef struct {
   bool is_save;
   bool is_search;
   bool is_recurse;
   bool show_path;
   string fname;
   string search_str;
   string path_str;
   string cmd_str;
   string cmd_pwd;
   string argv0;
   search_e search_mode;
   vector<string> skip_list;
   unordered_map<string, vector<string>> cmd_map;
} globals_t;

static void usage(const char *cmd)
{
cout << "usage goes here" << endl;
   exit(-1);
   cout << cmd << " : command to save or search command lines" << endl;
   cout << "setup:" << endl;
   cout << " make sure " << cmd << " is in your path" << endl;
   cout << " in .bashrc, add:" << endl;
   cout << "\t" << "trap 'cmd=$BASH_COMMAND; h $cmd' DEBUG" << endl;
   cout << "\t" << cmd << " -s | [search string]" << endl;
   exit(-1);
}
//trap 'cmd=$BASH_COMMAND; h $cmd' DEBUG

static bool is_path(const char *path)
{
   if (path[0] == '/')
      return true;
   if (path[0] == '.')
      return true;
   if ((path[0] == '~') && (path[1] == '/'))
      return true;

   return false;
}

static string make_path(const char *path)
{
   if (path[0] == '/')
      return string(path);

   if (path[0] == '.') {
      string fpath = getenv("PWD");
      if ((strlen(path) > 1) && (path[1] == '/'))
         fpath += &path[1];
      return fpath;
   }

   if (path[0] == '~') {
      string fpath = getenv("HOME");
      fpath += &path[1];
      return fpath;
   }
   return "";
}

static string make_cmd(int argc, char **argv)
{
   string cmd;
   for(int i=2;i<argc;i++) {
      cmd += argv[i];
      cmd += " ";
   }
   return cmd;
}

static void process_cmdline(globals_t *g, int argc, char **argv)
{
   int c;
   g->argv0 = argv[0];
   g->cmd_pwd = getenv("PWD");

   // getopt doesn't seem to work when called from a bash trap, parse manually
   string sopt = argv[1];
   if (sopt == "-s") {
      g->is_save = true;
      g->is_search = false; 
      g->cmd_str = make_cmd(argc, argv);
      return;
   }

   // parse the command options 
   while ( (c = getopt(argc, argv, "srph")) != -1 ) {
      switch ( c ) {
      case 's': g->is_save = true; 
                g->is_search = false; 
                break;
      case 'r': g->is_recurse = true; break;
      case 'p': g->show_path = true; break;
      case 'h':
      default:
         usage(argv[0]);
      }
   }

   // handle -s (for test, the live one is above)
   if (g->is_save) {
      if (argc < 3)
         usage(argv[0]);
      g->cmd_str = make_cmd(argc, argv);
      return;
   }

   // last arg is path or search
   if (argc - optind == 1) {
      if (is_path(argv[optind]))
         g->path_str = make_path(argv[optind]);
      else
         g->search_str = argv[optind];
      return;
   }

   // last arg is path and search
   if (argc - optind == 2) {
      if (!is_path(argv[optind])) {
         cout << "path string: " << g->path_str << " must start with '/' or '~/' or '.' " << endl;
         exit(-1);
      }
      g->path_str = make_path(argv[optind]);
      g->search_str = argv[optind+1];
      return;
   }
   usage(argv[0]);
}

static void init_globals(globals_t *g)
{
   char *fname = getenv("MH_STORE");
   if (!fname) 
      exit(0);

   g->fname = fname;
   g->is_search = true;

   g->is_save = false;
   g->is_recurse = false;
   g->show_path = false;
   g->path_str.clear();
   g->search_str.clear();
   g->cmd_str.clear();
}


static void dump_map(globals_t *g)
{
   for ( auto it : g->cmd_map) {
      cout << it.first << endl;
      for ( auto cmd : it.second ) {
         cout << "  " << cmd << endl;
      }
   }
}

static bool is_present(globals_t *g, const string & path, const string & cmd)
{
   // check path
   auto it = g->cmd_map.find(path);
   if (it == g->cmd_map.end())
      return false;

   // check command
   auto it2 = find(it->second.begin(), it->second.end(), cmd);
   if (it2 == it->second.end())
      return false;

   return true;
}

static void process_load(globals_t *g)
{
   ifstream infile(g->fname);
   if (!infile.is_open()) {
      exit(-1);
   }

   // load the file into the map of paths and cmds
   while (infile.good()) {
      string str;
      getline(infile, str);
      if (str.length()) {
         size_t pos = str.find("\036");
         g->cmd_map[str.substr(0,pos-1)].push_back(str.substr(pos+1));
      }
   }
   infile.close();
}

static void process_save(globals_t *g)
{
   if (!g->cmd_str.compare(0,2,"h "))
      return;

   // check if on skip list


   // check is duplicate
   process_load(g);
   if (is_present(g, g->cmd_pwd, g->cmd_str)) return;
   
   ofstream outfile(g->fname, ios_base::app);
   if (!outfile.is_open()) {
      cout << "could not open " << g->fname << endl;
      exit(-1);
   }
cout << "add: " << g->cmd_pwd << " : " << g->cmd_str << endl;
   outfile << g->cmd_pwd << " \036" << g->cmd_str << endl;
   outfile.close();
}


static search_e set_search(globals_t *g)
{
   bool have_search  = !g->search_str.empty();
   bool have_path    = !g->path_str.empty();

   if (have_search && !g->show_path && !have_path)
      return e_simple;

   if (have_search && g->show_path && !have_path)
      return e_showpaths;

   if (have_search && have_path && !g->is_recurse)
      return e_onepath;

   if (have_search && have_path && g->is_recurse)
      return e_onepath_r;

   if (!have_search && have_path && !g->is_recurse)
      return e_onepath_all;

   if (!have_search && have_path && g->is_recurse)
      return e_onepath_all_r;

   return e_null;
}

static void process_search(globals_t *g)
{
   process_load(g);

   // create the regex string against the whole command line
   string match = "(.*)("; 
   match += g->search_str;
   match += ")(.*)";

   // create a replacement to print the search string in red
   string replace = "\033[1;31m";
   replace += g->search_str;
   replace += "\033[0m";

   // load file
   switch(set_search(g)) {

      // creates a list of matches regardless of path, shows only unique ones
      case e_simple:
      {
         vector<string> unique_list;
         for ( auto it : g->cmd_map) {
            for ( auto cmd : it.second ) {
               if (regex_match(cmd, regex(match))) {
                  auto it = find(unique_list.begin(), unique_list.end(), cmd);
                  if (it == unique_list.end())
                     unique_list.push_back(cmd);
               }
            }
         }
         for (auto found : unique_list)
            cout << regex_replace(found, regex(g->search_str), replace) << endl;
      }
      break;

      // shows matching commands along with the path, which may include duplicates
      case e_showpaths:
      {
         for ( auto it : g->cmd_map) {
            for ( auto cmd : it.second ) {
               if (regex_match(cmd, regex(match))) {
                  cout << it.first << ": " << regex_replace(cmd, regex(g->search_str), replace) << endl;
               }
            }
         }
      }
      break;

      // show matching commands for a specific path only
      case e_onepath:
      {
         auto it = g->cmd_map.find(g->path_str);
         if (it == g->cmd_map.end()) {
            cout << "path " << g->path_str << " not found" << endl;
            break;
         }
         for ( auto cmd : it->second ) {
            if (regex_match(cmd, regex(match))) {
               cout << regex_replace(cmd, regex(g->search_str), replace) << endl;
            }
         }
      }
      break;

      // show matching commands for a specific path and recursed
      case e_onepath_r:
      {
         string redpath = "\033[1;31m";
         redpath += g->path_str;
         redpath += "\033[0m";
         if (g->cmd_map.find(g->path_str) == g->cmd_map.end()) {
            cout << "path " << redpath << " not found" << endl;
            break;
         }
         for ( auto it : g->cmd_map) {
            size_t pos = it.first.find(g->path_str);
            if (pos == string::npos)
               continue;
            string subpath = it.first;
            for ( auto cmd : it.second ) {
               if (regex_match(cmd, regex(match))) {
                  cout << subpath.replace(0, g->path_str.size(), redpath) << ": ";
                  cout << regex_replace(cmd, regex(g->search_str), replace) << endl;
               }
            }
         }
      }
      break;

      // show all commands for a specific path
      case e_onepath_all:
      {
         auto it = g->cmd_map.find(g->path_str);
         if (it == g->cmd_map.end()) {
            cout << "path " << g->path_str << " not found" << endl;
            break;
         }
         for ( auto cmd : it->second ) {
            cout << "\033[1;31m" << g->path_str << "\033[0m" << ": " << cmd << endl;
         }
      }
      break;

      // show all commands for a specific path recursively
      case e_onepath_all_r:
      {
         string redpath = "\033[1;31m";
         redpath += g->path_str;
         redpath += "\033[0m";
         if (g->cmd_map.find(g->path_str) == g->cmd_map.end()) {
            cout << "path " << redpath << " not found" << endl;
            break;
         }
         for ( auto it : g->cmd_map) {
            size_t pos = it.first.find(g->path_str);
            if (pos == string::npos)
               continue;
            string subpath = it.first;
            cout << subpath.replace(0, g->path_str.size(), redpath) << endl;
            for ( auto cmd : it.second ) {
               cout << "  " << cmd << endl;
            }
         }
      }
      break;
      default:
         fprintf(stderr, "could not determine search mode\n");
         usage(g->argv0.c_str());
      break;
   }
}

int main(int argc, char **argv)
{
   globals_t g;

   if (argc == 1)
      usage(argv[0]);

   init_globals(&g);
   process_cmdline(&g, argc, argv); 

   if (g.is_save) 
      process_save(&g);
   else
      process_search(&g);

   return 0;
   dump_map(&g);
   return 0;
}
