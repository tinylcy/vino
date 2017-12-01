#!/usr/bin/perl -w
use strict;

sub help()
{
    print <<END;
cinclude2dot v1.1 (C) Darxus\@ChaosReigns.com, francis\@flourish.org
Released under the terms of the GNU General Public license.
Download from http://www.flourish.org/cinclude2dot/
or original version http://www.chaosreigns.com/code/cinclude2dot/

Graphs #include relationships between every C/C++ source and header file
under the current directory using graphviz like so:

\$ cd ~/program/src
\$ ./cinclude2dot.pl > source.dot
\$ neato -Tps source.dot > source.ps
\$ gv source.ps

You might like to try "dot" instead of "neato" as well.

Get graphviz here: http://www.research.att.com/sw/tools/graphviz/

Command line options are:

--debug       Display various debug info
--exclude     Specify a regular expression of filenames to ignore
              For example, ignore your test harnesses.
--merge       Granularity of the diagram:
                 file - the default, treats each file as separate
                 module - merges .c/.cc/.cpp/.cxx and .h/.hpp/.hxx pairs
                 directory - merges directories into one node
--groups      Cluster files or modules into directory groups
--help        Display this help page.
--include     Followed by a comma separated list of include search
                 paths.
--paths       Leaves relative paths in displayed filenames.
--quotetypes  Select for parsing the files included by strip quotes or angle brackets:
                 both - the default, parse all headers.
                 angle - include only "system" headers included by anglebrackets (<>)
                 quote - include only "user" headers included by strip quotes ("")
--src         Followed by a path to the source code, defaults to current directory

END
    exit 0;
}
 
##########################################################################
# Version history:
#
# v0.1 2000-09-14 10:24 initial version
# v0.2 2000-09-14 10:49 strip leading ./ off files
# v0.3 2000-09-14 11:00 allow whitespace between "^#" and "include"
# v0.4 2000-09-14 12:44 strip paths
# v0.5 2000-09-14 12:57 strip paths unless -p
# v0.6 2002-02-08 10:30 major changes by francis@flourish.org
# v0.7 2002-05-10 01:00 changed from a fixed len=5 to dynamic overlap=scale (Darxus)
# v0.8 2003-05-26 19:48 patch from Preston Bannister for use with cygwin
# v0.9 2003-12-10 14:52 Solaris compatible "find" call from chatko
#      2004-03-18 11:00 added "--quotetypes" options by Viktor Chyzhdzenka (chyzhdzenka@mail.ru)
# v1.0 2004-04-13 00:11 use File::Find rather than the shell `find` for win32 compat
# v1.1 2005-12-13 02:05 add --src and .C (by Matthijs den Besten)

##########################################################################
# Ideas for improvements:
# 
# - Change the %links hash to store a count, and weigh the link by how often
#   there was an include along that link.  Thicker lines mean stronger
#   connections.
#
# - Add label to digraph, saying which program generated it, when and guess
#   at the name of the application which has been graphed.
#
# - Write links from the same node in one line, rather than several.
#   "putmpg.c" -> { "config.h" "global.h" "stdio.h" "stdio.h" }
#

use Getopt::Long;
use File::Spec::Functions;
use File::Basename;
use File::Find;

##########################################################################
# Read parameters

my $paths = '';
my $help ='';
my $debug = '';
my $groups = '';
my $exclude = "";
my $merge = "file";
my $includelist = "";
my $quotetypes = "both";
my $src = '.';
die if !GetOptions('paths' => \$paths, 'groups' => \$groups,
           'exclude=s' => \$exclude, 'merge=s' => \$merge,
           'include=s' => \$includelist, 'help' =>\$help,
           'debug' => \$debug, 'quotetypes=s' => \$quotetypes,
           'src=s' => \$src);
my @includepaths = split(/,/,$includelist);
help() if ($help);
if ($merge ne "file" && $merge ne "module" && $merge ne "directory")
{
    die "Cluster parameter must be one of file, module or directory";
}
if ($quotetypes ne "both" && $quotetypes ne "angle" && $quotetypes ne "quote")
{
    die "quotetypes parameter must be one of both, angle or quote";
}

##########################################################################
# Tidies up a path or filename, removing excess ./ and ../ parts.
# Parameters:
#   $_ - Path or filename to tidy
sub tidypath {
    $_ = shift;
    $_ = canonpath($_);
    # Use only one type of slash
    s:\\:/:g;
    # Remove constructs like xyz/..
    while (s:[^/]+?/\.\./::) {};
    return $_;
}

##########################################################################
# Searches file include path, and returns exact filename of file
# Parameters: 
#   $inc - text from within the #include statement
#   $file - filename the include was from
sub include_search {
    my ($inc, $file) = @_;
    warn "include_search scanning for $inc from $file" if $debug;

    # Try relative to the including file
    $_ = tidypath(dirname($file) . "/" . $inc);
    warn "include_search trying $_ (dirname " . dirname($file) . ")" if $debug;
    return $_ if (-e "$_");

    # Try user-specified include paths
    my $item;
    foreach $item (@includepaths)
    {
      $_ = tidypath($item . "/" . $inc);
      warn "include_search trying $_" if $debug;
      return $_ if (-e "$_");
    }

    # Try relative to current directory
    $_ = $inc;
    warn "include_search trying $_" if $debug;
    return $_ if (-e "$_");

    warn "include_search failed for $inc from $file" if $debug;
    return undef;
}

##########################################################################
# Converts a filename to its display version
# Parameters:
#   $_ - Filename for display
sub file_display {
    $_ = shift;
    $_ = basename($_) unless $paths;
    if ($merge eq "module")
    {
        s/\.c$//; 
        s/\.cc$//; 
        s/\.cxx$//;
        s/\.cpp$//;
        s/\.C$//; 
        s/\.h$//; 
        s/\.hpp$//;
        s/\.hxx$//;
    }
    s:/:/\\n:g;
    return $_;
}

##########################################################################
# Main code

my @files;
find sub { push @files, $File::Find::name if /\.(c|cc|cxx|cpp|C|h|hpp|hxx)$/ }, $src;

print <<END;
digraph "source tree" {
    overlap=scale;
    size="8,10";
    ratio="fill";
    fontsize="16";
    fontname="Helvetica";
    clusterrank="local";
END
#  nodesep="0.2";
#    ranksep="0.3";
#    margin=".5,.5";
#    orientation="portrait";

my %links = ();
my %notfound = ();
my $file;
foreach $file (@files)
{
  next if ($exclude ne "" and $file =~ m/$exclude/);
    
  chomp $file;
  #strip "./" off off $file
  $file =~ s#^./##g;

  open (INPUT,"<$file");
  my $line;
  while ($line = <INPUT>)
  {
#    if ($line =~ m#^\#\s*include\s(\S+)#)
    my $regexp_include_line = '^\#\s*include\s+(\S+)';
    if ($quotetypes eq "angle") 
        { $regexp_include_line = '^\#\s*include\s+<(\S+)>'; }
    elsif ($quotetypes eq "quote") 
        { $regexp_include_line = '^\#\s*include\s+"(\S+)"'; }
    if ($line =~ m/$regexp_include_line/)
    {
      my $included = $1;

      # strip quotes & anglebrackets
      (my $rawincluded = $included) =~ s/[\<\>"]//g;
      my $includefile = include_search($rawincluded, $file);

      if (! defined $includefile)
      {
          $notfound{$included . " from " . $file} = 1;
          next;
      }

      if ($merge eq "directory")
      {
          my $from = dirname($file);
          my $to = dirname($includefile);
          $links{"  \"$from\" -> \"$to\"\n"} = 1 unless $from eq $to;
      }
      else
      {
          my $includefile_display = file_display($includefile);
          my $file_display = file_display($file);

          if ($groups)
          {
              my $groupname = dirname($includefile);
              print <<END;
subgraph "cluster$groupname" {
    label="$groupname";
    "$includefile_display";
}
END
              $groupname = dirname($file);
              print <<END;
subgraph "cluster$groupname" {
    label="$groupname";
    "$file_display";
}
END
          }

          if ($file_display ne $includefile_display)
          {
              $links{"  \"$file_display\" -> \"$includefile_display\"\n"} = 1;
          }
       }    
    }
  }

  close INPUT;
}

# Print out the links (we used a hash to avoid repeats of the same link)
my $item;
foreach $item (keys %links)
{
    print $item;
}

# Print names of not found files
foreach $item (sort keys %notfound)
{
    print STDERR "Include file not found: " . $item . "\n";
}

print "}\n";
