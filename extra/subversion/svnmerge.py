#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) 2005, Giovanni Bajo
# Copyright (c) 2004-2005, Awarix, Inc.
# All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
#
# Author: Archie Cobbs <archie at awarix dot com>
# Rewritten in Python by: Giovanni Bajo <rasky at develer dot com>
#
# Acknowledgments:
#   John Belmonte <john at neggie dot net> - metadata and usability
#     improvements
#   Blair Zajac <blair at orcaware dot com> - random improvements
#   Raman Gupta <rocketraman at fastmail dot fm> - bidirectional and transitive
#     merging support
#   Dustin J. Mitchell <dustin at zmanda dot com> - support for multiple
#     location identifier formats
#
# $HeadURL$
# $LastChangedDate$
# $LastChangedBy$
# $LastChangedRevision$
#
# Requisites:
# svnmerge.py has been tested with all SVN major versions since 1.1 (both
# client and server). It is unknown if it works with previous versions.
#
# Differences from svnmerge.sh:
# - More portable: tested as working in FreeBSD and OS/2.
# - Add double-verbose mode, which shows every svn command executed (-v -v).
# - "svnmerge avail" now only shows commits in source, not also commits in
#   other parts of the repository.
# - Add "svnmerge block" to flag some revisions as blocked, so that
#   they will not show up anymore in the available list.  Added also
#   the complementary "svnmerge unblock".
# - "svnmerge avail" has grown two new options:
#   -B to display a list of the blocked revisions
#   -A to display both the blocked and the available revisions.
# - Improved generated commit message to make it machine parsable even when
#   merging commits which are themselves merges.
# - Add --force option to skip working copy check
# - Add --record-only option to "svnmerge merge" to avoid performing
#   an actual merge, yet record that a merge happened.
# - Can use a variety of location-identifier formats
#
# TODO:
#  - Add "svnmerge avail -R": show logs in reverse order
#
# Information for Hackers:
#
# Identifiers for branches:
#  A branch is identified in three ways within this source:
#  - as a working copy (variable name usually includes 'dir')
#  - as a fully qualified URL
#  - as a path identifier (an opaque string indicating a particular path
#    in a particular repository; variable name includes 'pathid')
#  A "target" is generally user-specified, and may be a working copy or
#  a URL.

import sys, os, getopt, re, types, tempfile, time, locale
from bisect import bisect
from xml.dom import pulldom

NAME = "svnmerge"
if not hasattr(sys, "version_info") or sys.version_info < (2, 0):
    error("requires Python 2.0 or newer")

# Set up the separator used to separate individual log messages from
# each revision merged into the target location.  Also, create a
# regular expression that will find this same separator in already
# committed log messages, so that the separator used for this run of
# svnmerge.py will have one more LOG_SEPARATOR appended to the longest
# separator found in all the commits.
LOG_SEPARATOR = 8 * '.'
LOG_SEPARATOR_RE = re.compile('^((%s)+)' % re.escape(LOG_SEPARATOR),
                              re.MULTILINE)

# Each line of the embedded log messages will be prefixed by LOG_LINE_PREFIX.
LOG_LINE_PREFIX = 2 * ' '

# Set python to the default locale as per environment settings, same as svn
# TODO we should really parse config and if log-encoding is specified, set
# the locale to match that encoding
locale.setlocale(locale.LC_ALL, '')

# We want the svn output (such as svn info) to be non-localized
# Using LC_MESSAGES should not affect localized output of svn log, for example
if os.environ.has_key("LC_ALL"):
    del os.environ["LC_ALL"]
os.environ["LC_MESSAGES"] = "C"

###############################################################################
# Support for older Python versions
###############################################################################

# True/False constants are Python 2.2+
try:
    True, False
except NameError:
    True, False = 1, 0

def lstrip(s, ch):
    """Replacement for str.lstrip (support for arbitrary chars to strip was
    added in Python 2.2.2)."""
    i = 0
    try:
        while s[i] == ch:
            i = i+1
        return s[i:]
    except IndexError:
        return ""

def rstrip(s, ch):
    """Replacement for str.rstrip (support for arbitrary chars to strip was
    added in Python 2.2.2)."""
    try:
        if s[-1] != ch:
            return s
        i = -2
        while s[i] == ch:
            i = i-1
        return s[:i+1]
    except IndexError:
        return ""

def strip(s, ch):
    """Replacement for str.strip (support for arbitrary chars to strip was
    added in Python 2.2.2)."""
    return lstrip(rstrip(s, ch), ch)

def rsplit(s, sep, maxsplits=0):
    """Like str.rsplit, which is Python 2.4+ only."""
    L = s.split(sep)
    if not 0 < maxsplits <= len(L):
        return L
    return [sep.join(L[0:-maxsplits])] + L[-maxsplits:]

###############################################################################

def kwextract(s):
    """Extract info from a svn keyword string."""
    try:
        return strip(s, "$").strip().split(": ")[1]
    except IndexError:
        return "<unknown>"

__revision__ = kwextract('$Rev$')
__date__ = kwextract('$Date$')

# Additional options, not (yet?) mapped to command line flags
default_opts = {
    "svn": "svn",
    "prop": NAME + "-integrated",
    "block-prop": NAME + "-blocked",
    "commit-verbose": True,
    "verbose": 0,
}
logs = {}

def console_width():
    """Get the width of the console screen (if any)."""
    try:
        return int(os.environ["COLUMNS"])
    except (KeyError, ValueError):
        pass

    try:
        # Call the Windows API (requires ctypes library)
        from ctypes import windll, create_string_buffer
        h = windll.kernel32.GetStdHandle(-11)
        csbi = create_string_buffer(22)
        res = windll.kernel32.GetConsoleScreenBufferInfo(h, csbi)
        if res:
            import struct
            (bufx, bufy,
             curx, cury, wattr,
             left, top, right, bottom,
             maxx, maxy) = struct.unpack("hhhhHhhhhhh", csbi.raw)
            return right - left + 1
    except ImportError:
        pass

    # Parse the output of stty -a
    if os.isatty(1):
        out = os.popen("stty -a").read()
        m = re.search(r"columns (\d+);", out)
        if m:
            return int(m.group(1))

    # sensible default
    return 80

def error(s):
    """Subroutine to output an error and bail."""
    print >> sys.stderr, "%s: %s" % (NAME, s)
    sys.exit(1)

def report(s):
    """Subroutine to output progress message, unless in quiet mode."""
    if opts["verbose"]:
        print "%s: %s" % (NAME, s)

def prefix_lines(prefix, lines):
    """Given a string representing one or more lines of text, insert the
    specified prefix at the beginning of each line, and return the result.
    The input must be terminated by a newline."""
    assert lines[-1] == "\n"
    return prefix + lines[:-1].replace("\n", "\n"+prefix) + "\n"

def recode_stdout_to_file(s):
    if locale.getdefaultlocale()[1] is None or not hasattr(sys.stdout, "encoding") \
            or sys.stdout.encoding is None:
        return s
    u = s.decode(sys.stdout.encoding)
    return u.encode(locale.getdefaultlocale()[1])

class LaunchError(Exception):
    """Signal a failure in execution of an external command. Parameters are the
    exit code of the process, the original command line, and the output of the
    command."""

try:
    """Launch a sub-process. Return its output (both stdout and stderr),
    optionally split by lines (if split_lines is True). Raise a LaunchError
    exception if the exit code of the process is non-zero (failure).

    This function has two implementations, one based on subprocess (preferred),
    and one based on popen (for compatibility).
    """
    import subprocess
    import shlex

    def launch(cmd, split_lines=True):
        # Requiring python 2.4 or higher, on some platforms we get
        # much faster performance from the subprocess module (where python
        # doesn't try to close an exhorbitant number of file descriptors)
        stdout = ""
        stderr = ""
        try:
            if os.name == 'nt':
                p = subprocess.Popen(cmd, stdout=subprocess.PIPE, \
                                     close_fds=False, stderr=subprocess.PIPE)
            else:
                # Use shlex to break up the parameters intelligently,
                # respecting quotes. shlex can't handle unicode.
                args = shlex.split(cmd.encode('ascii'))
                p = subprocess.Popen(args, stdout=subprocess.PIPE, \
                                     close_fds=False, stderr=subprocess.PIPE)
            stdoutAndErr = p.communicate()
            stdout = stdoutAndErr[0]
            stderr = stdoutAndErr[1]
        except OSError, inst:
            # Using 1 as failure code; should get actual number somehow? For
            # examples see svnmerge_test.py's TestCase_launch.test_failure and
            # TestCase_launch.test_failurecode.
            raise LaunchError(1, cmd, stdout + " " + stderr + ": " + str(inst))

        if p.returncode == 0:
            if split_lines:
                # Setting keepends=True for compatibility with previous logic
                # (where file.readlines() preserves newlines)
                return stdout.splitlines(True)
            else:
                return stdout
        else:
            raise LaunchError(p.returncode, cmd, stdout + stderr)
except ImportError:
    # support versions of python before 2.4 (slower on some systems)
    def launch(cmd, split_lines=True):
        if os.name not in ['nt', 'os2']:
            import popen2
            p = popen2.Popen4(cmd)
            p.tochild.close()
            if split_lines:
                out = p.fromchild.readlines()
            else:
                out = p.fromchild.read()
            ret = p.wait()
            if ret == 0:
                ret = None
            else:
                ret >>= 8
        else:
            i,k = os.popen4(cmd)
            i.close()
            if split_lines:
                out = k.readlines()
            else:
                out = k.read()
            ret = k.close()

        if ret is None:
            return out
        raise LaunchError(ret, cmd, out)

def launchsvn(s, show=False, pretend=False, **kwargs):
    """Launch SVN and grab its output."""
    username = password = configdir = ""
    if opts.get("username", None):
        username = "--username=" + opts["username"]
    if opts.get("password", None):
        password = "--password=" + opts["password"]
    if opts.get("config-dir", None):
        configdir = "--config-dir=" + opts["config-dir"]
    cmd = ' '.join(filter(None, [opts["svn"], "--non-interactive",
                                 username, password, configdir, s]))
    if show or opts["verbose"] >= 2:
        print cmd
    if pretend:
        return None
    return launch(cmd, **kwargs)

def svn_command(s):
    """Do (or pretend to do) an SVN command."""
    out = launchsvn(s, show=opts["show-changes"] or opts["dry-run"],
                    pretend=opts["dry-run"],
                    split_lines=False)
    if not opts["dry-run"]:
        print out

def check_dir_clean(dir):
    """Check the current status of dir for local mods."""
    if opts["force"]:
        report('skipping status check because of --force')
        return
    report('checking status of "%s"' % dir)

    # Checking with -q does not show unversioned files or external
    # directories.  Though it displays a debug message for external
    # directories, after a blank line.  So, practically, the first line
    # matters: if it's non-empty there is a modification.
    out = launchsvn("status -q %s" % dir)
    if out and out[0].strip():
        error('"%s" has local modifications; it must be clean' % dir)

class PathIdentifier:
    """Abstraction for a path identifier, so that we can start talking
    about it before we know the form that it takes in the properties (its
    external_form).  Objects are referenced in the class variable 'locobjs',
    keyed by all known forms."""
    
    # a map of UUID (or None) to repository root URL.
    repo_hints = {}

    # a map from any known string form to the corresponding PathIdentifier
    locobjs = {}

    def __init__(self, repo_relative_path, uuid=None, url=None, external_form=None):
        self.repo_relative_path = repo_relative_path
        self.uuid = uuid
        self.url = url
        self.external_form = external_form

    def __repr__(self):
        return "<PathIdentifier " + ', '.join('%s=%r' % i for i in self.__dict__.items()) + '>'

    def __str__(self):
        """Return a printable string representation"""
        if self.external_form:
            return self.external_form
        if self.url:
            return self.format('url')
        if self.uuid:
            return self.format('uuid')
        return self.format('path')

    def from_pathid(pathid_str):
        """convert pathid_str to a PathIdentifier"""
        if not PathIdentifier.locobjs.has_key(pathid_str):
            if is_url(pathid_str):
                # we can determine every form; PathIdentifier.hint knows how to do that
                PathIdentifier.hint(pathid_str)
            elif pathid_str[:7] == 'uuid://':
                mo = re.match('uuid://([^/]*)(.*)', pathid_str)
                if not mo:
                    error("Invalid path identifier '%s'" % pathid_str)
                uuid, repo_relative_path = mo.groups()
                pathid = PathIdentifier(repo_relative_path, uuid=uuid)
                # we can cache this by uuid:// pathid and by repo-relative path
                PathIdentifier.locobjs[pathid_str] = PathIdentifier.locobjs[repo_relative_path] = pathid
            elif pathid_str and pathid_str[0] == '/':
                # strip any trailing slashes
                pathid_str = pathid_str.rstrip('/')
                pathid = PathIdentifier(repo_relative_path=pathid_str)
                # we can only cache this by repo-relative path
                PathIdentifier.locobjs[pathid_str] = pathid
            else:
                error("Invalid path identifier '%s'" % pathid_str)
        return PathIdentifier.locobjs[pathid_str]
    from_pathid = staticmethod(from_pathid)

    def from_target(target):
        """Convert a target (either a working copy path or an URL) into a
        path identifier."""
        # prime the cache first if we don't know about this target yet
        if not PathIdentifier.locobjs.has_key(target):
            PathIdentifier.hint(target)

        try:
            return PathIdentifier.locobjs[target]
        except KeyError:
            error("Could not recognize path identifier '%s'" % target)
    from_target = staticmethod(from_target)

    def hint(target):
        """Cache some information about target, as it may be referenced by
        repo-relative path in subversion properties; the cache can help to
        expand such a relative path to a full path identifier."""
        if PathIdentifier.locobjs.has_key(target): return
        if not is_url(target) and not is_wc(target): return

        url = target_to_url(target)

        root = get_repo_root(url)
        assert root[-1] != "/"
        assert url[:len(root)] == root, "url=%r, root=%r" % (url, root)
        repo_relative_path = url[len(root):]

        try:
            uuid = get_svninfo(target)['Repository UUID']
            uuid_pathid = 'uuid://%s%s' % (uuid, repo_relative_path)
        except KeyError:
            uuid = None
            uuid_pathid = None

        locobj = PathIdentifier.locobjs.get(url) or \
                 (uuid_pathid and PathIdentifier.locobjs.get(uuid_pathid))
        if not locobj:
            locobj = PathIdentifier(repo_relative_path, uuid=uuid, url=url)

        PathIdentifier.repo_hints[uuid] = root # (uuid may be None)

        PathIdentifier.locobjs[target] = locobj
        PathIdentifier.locobjs[url] = locobj
        if uuid_pathid:
            PathIdentifier.locobjs[uuid_pathid] = locobj
        if not PathIdentifier.locobjs.has_key(repo_relative_path):
            PathIdentifier.locobjs[repo_relative_path] = locobj
    hint = staticmethod(hint)

    def format(self, fmt):
        if fmt == 'path':
            return self.repo_relative_path
        elif fmt == 'uuid':
            return "uuid://%s%s" % (self.uuid, self.repo_relative_path)
        elif fmt == 'url':
            return self.url
        else:
            error("Unkonwn path type '%s'" % fmt)

    def match_substring(self, str):
        """Test whether str is a substring of any representation of this
        PathIdentifier."""
        if self.repo_relative_path.find(str) >= 0:
            return True

        if self.uuid:
            if ("uuid://%s%s" % (self.uuid, self.repo_relative_path)).find(str) >= 0:
                return True

        if self.url:
            if (self.url + self.repo_relative_path).find(str) >= 0:
                return True

        return False

    def get_url(self):
        """Convert a pathid into a URL.  If this is not possible, error out."""
        if self.url:
            return self.url
        # if we have a uuid and happen to know the URL for it, use that
        elif self.uuid and PathIdentifier.repo_hints.has_key(self.uuid):
            self.url = PathIdentifier.repo_hints[self.uuid] + self.repo_relative_path
            PathIdentifier.locobjs[self.url] = self
            return self.url
        # if we've only seen one rep, use that (a guess, but an educated one)
        elif not self.uuid and len(PathIdentifier.repo_hints) == 1:
            uuid, root = PathIdentifier.repo_hints.items()[0]
            if uuid:
                self.uuid = uuid
                PathIdentifier.locobjs['uuid://%s%s' % (uuid, self.repo_relative_path)] = self
            self.url = root + self.repo_relative_path
            PathIdentifier.locobjs[self.url] = self
            report("Guessing that '%s' refers to '%s'" % (self, self.url))
            return self.url
        else:
            error("Cannot determine URL for '%s'; " % self +
                  "Explicit source argument (-S/--source) required.\n")

class RevisionLog:
    """
    A log of the revisions which affected a given URL between two
    revisions.
    """

    def __init__(self, url, begin, end, find_propchanges=False):
        """
        Create a new RevisionLog object, which stores, in self.revs, a list
        of the revisions which affected the specified URL between begin and
        end. If find_propchanges is True, self.propchange_revs will contain a
        list of the revisions which changed properties directly on the
        specified URL. URL must be the URL for a directory in the repository.
        """
        self.url = url

        # Setup the log options (--quiet, so we don't show log messages)
        log_opts = '--xml --quiet -r%s:%s "%s"' % (begin, end, url)
        if find_propchanges:
            # The --verbose flag lets us grab merge tracking information
            # by looking at propchanges
            log_opts = "--verbose " + log_opts

        # Read the log to look for revision numbers and merge-tracking info
        self.revs = []
        self.propchange_revs = []
        repos_pathid = PathIdentifier.from_target(url)
        for chg in SvnLogParser(launchsvn("log %s" % log_opts,
                                          split_lines=False)):
            self.revs.append(chg.revision())
            for p in chg.paths():
                if p.action() == 'M' and p.pathid() == repos_pathid.repo_relative_path:
                    self.propchange_revs.append(chg.revision())

        # Save the range of the log
        self.begin = int(begin)
        if end == "HEAD":
            # If end is not provided, we do not know which is the latest
            # revision in the repository. So we set 'end' to the latest
            # known revision.
            self.end = self.revs[-1]
        else:
            self.end = int(end)

        self._merges = None
        self._blocks = None

    def merge_metadata(self):
        """
        Return a VersionedProperty object, with a cached view of the merge
        metadata in the range of this log.
        """

        # Load merge metadata if necessary
        if not self._merges:
            self._merges = VersionedProperty(self.url, opts["prop"])
            self._merges.load(self)

        return self._merges

    def block_metadata(self):
        if not self._blocks:
            self._blocks = VersionedProperty(self.url, opts["block-prop"])
            self._blocks.load(self)

        return self._blocks


class VersionedProperty:
    """
    A read-only, cached view of a versioned property.

    self.revs contains a list of the revisions in which the property changes.
    self.values stores the new values at each corresponding revision. If the
    value of the property is unknown, it is set to None.

    Initially, we set self.revs to [0] and self.values to [None]. This
    indicates that, as of revision zero, we know nothing about the value of
    the property.

    Later, if you run self.load(log), we cache the value of this property over
    the entire range of the log by noting each revision in which the property
    was changed. At the end of the range of the log, we invalidate our cache
    by adding the value "None" to our cache for any revisions which fall out
    of the range of our log.

    Once self.revs and self.values are filled, we can find the value of the
    property at any arbitrary revision using a binary search on self.revs.
    Once we find the last revision during which the property was changed,
    we can lookup the associated value in self.values. (If the associated
    value is None, the associated value was not cached and we have to do
    a full propget.)

    An example: We know that the 'svnmerge' property was added in r10, and
    changed in r21. We gathered log info up until r40.

    revs = [0, 10, 21, 40]
    values = [None, "val1", "val2", None]

    What these values say:
    - From r0 to r9, we know nothing about the property.
    - In r10, the property was set to "val1". This property stayed the same
      until r21, when it was changed to "val2".
    - We don't know what happened after r40.
    """

    def __init__(self, url, name):
        """View the history of a versioned property at URL with name"""
        self.url = url
        self.name = name

        # We know nothing about the value of the property. Setup revs
        # and values to indicate as such.
        self.revs = [0]
        self.values = [None]

        # We don't have any revisions cached
        self._initial_value = None
        self._changed_revs = []
        self._changed_values = []

    def load(self, log):
        """
        Load the history of property changes from the specified
        RevisionLog object.
        """

        # Get the property value before the range of the log
        if log.begin > 1:
            self.revs.append(log.begin-1)
            try:
                self._initial_value = self.raw_get(log.begin-1)
            except LaunchError:
                # The specified URL might not exist before the
                # range of the log. If so, we can safely assume
                # that the property was empty at that time.
                self._initial_value = { }
            self.values.append(self._initial_value)
        else:
            self._initial_value = { }
            self.values[0] = self._initial_value

        # Cache the property values in the log range
        old_value = self._initial_value
        for rev in log.propchange_revs:
            new_value = self.raw_get(rev)
            if new_value != old_value:
                self._changed_revs.append(rev)
                self._changed_values.append(new_value)
                self.revs.append(rev)
                self.values.append(new_value)
                old_value = new_value

        # Indicate that we know nothing about the value of the property
        # after the range of the log.
        if log.revs:
            self.revs.append(log.end+1)
            self.values.append(None)

    def raw_get(self, rev=None):
        """
        Get the property at revision REV. If rev is not specified, get
        the property at revision HEAD.
        """
        return get_revlist_prop(self.url, self.name, rev)

    def get(self, rev=None):
        """
        Get the property at revision REV. If rev is not specified, get
        the property at revision HEAD.
        """

        if rev is not None:

            # Find the index using a binary search
            i = bisect(self.revs, rev) - 1

            # Return the value of the property, if it was cached
            if self.values[i] is not None:
                return self.values[i]

        # Get the current value of the property
        return self.raw_get(rev)

    def changed_revs(self, key=None):
        """
        Get a list of the revisions in which the specified dictionary
        key was changed in this property. If key is not specified,
        return a list of revisions in which any key was changed.
        """
        if key is None:
            return self._changed_revs
        else:
            changed_revs = []
            old_val = self._initial_value
            for rev, val in zip(self._changed_revs, self._changed_values):
                if val.get(key) != old_val.get(key):
                    changed_revs.append(rev)
                    old_val = val
            return changed_revs

    def initialized_revs(self):
        """
        Get a list of the revisions in which keys were added or
        removed in this property.
        """
        initialized_revs = []
        old_len = len(self._initial_value)
        for rev, val in zip(self._changed_revs, self._changed_values):
            if len(val) != old_len:
                initialized_revs.append(rev)
                old_len = len(val)
        return initialized_revs

class RevisionSet:
    """
    A set of revisions, held in dictionary form for easy manipulation. If we
    were to rewrite this script for Python 2.3+, we would subclass this from
    set (or UserSet).  As this class does not include branch
    information, it's assumed that one instance will be used per
    branch.
    """
    def __init__(self, parm):
        """Constructs a RevisionSet from a string in property form, or from
        a dictionary whose keys are the revisions. Raises ValueError if the
        input string is invalid."""

        self._revs = {}

        revision_range_split_re = re.compile('[-:]')

        if isinstance(parm, types.DictType):
            self._revs = parm.copy()
        elif isinstance(parm, types.ListType):
            for R in parm:
                self._revs[int(R)] = 1
        else:
            parm = parm.strip()
            if parm:
                for R in parm.split(","):
                    rev_or_revs = re.split(revision_range_split_re, R)
                    if len(rev_or_revs) == 1:
                        self._revs[int(rev_or_revs[0])] = 1
                    elif len(rev_or_revs) == 2:
                        for rev in range(int(rev_or_revs[0]),
                                         int(rev_or_revs[1])+1):
                            self._revs[rev] = 1
                    else:
                        raise ValueError, 'Ill formatted revision range: ' + R

    def sorted(self):
        revnums = self._revs.keys()
        revnums.sort()
        return revnums

    def normalized(self):
        """Returns a normalized version of the revision set, which is an
        ordered list of couples (start,end), with the minimum number of
        intervals."""
        revnums = self.sorted()
        revnums.reverse()
        ret = []
        while revnums:
            s = e = revnums.pop()
            while revnums and revnums[-1] in (e, e+1):
                e = revnums.pop()
            ret.append((s, e))
        return ret

    def __str__(self):
        """Convert the revision set to a string, using its normalized form."""
        L = []
        for s,e in self.normalized():
            if s == e:
                L.append(str(s))
            else:
                L.append(str(s) + "-" + str(e))
        return ",".join(L)

    def __contains__(self, rev):
        return self._revs.has_key(rev)

    def __sub__(self, rs):
        """Compute subtraction as in sets."""
        revs = {}
        for r in self._revs.keys():
            if r not in rs:
                revs[r] = 1
        return RevisionSet(revs)

    def __and__(self, rs):
        """Compute intersections as in sets."""
        revs = {}
        for r in self._revs.keys():
            if r in rs:
                revs[r] = 1
        return RevisionSet(revs)

    def __nonzero__(self):
        return len(self._revs) != 0

    def __len__(self):
        """Return the number of revisions in the set."""
        return len(self._revs)

    def __iter__(self):
        return iter(self.sorted())

    def __or__(self, rs):
        """Compute set union."""
        revs = self._revs.copy()
        revs.update(rs._revs)
        return RevisionSet(revs)

def merge_props_to_revision_set(merge_props, pathid):
    """A converter which returns a RevisionSet instance containing the
    revisions from PATH as known to BRANCH_PROPS.  BRANCH_PROPS is a
    dictionary of pathid -> revision set branch integration information
    (as returned by get_merge_props())."""
    if not merge_props.has_key(pathid):
        error('no integration info available for path "%s"' % pathid)
    return RevisionSet(merge_props[pathid])

def dict_from_revlist_prop(propvalue):
    """Given a property value as a string containing per-source revision
    lists, return a dictionary whose key is a source path identifier
    and whose value is the revisions for that source."""
    prop = {}

    # Multiple sources are separated by any whitespace.
    for L in propvalue.split():
        # We use rsplit to play safe and allow colons in pathids.
        pathid_str, revs = rsplit(L.strip(), ":", 1)

        pathid = PathIdentifier.from_pathid(pathid_str)

        # cache the "external" form we saw
        pathid.external_form = pathid_str

        prop[pathid] = revs
    return prop

def get_revlist_prop(url_or_dir, propname, rev=None):
    """Given a repository URL or working copy path and a property
    name, extract the values of the property which store per-source
    revision lists and return a dictionary whose key is a source path
    identifier, and whose value is the revisions for that source."""

    # Note that propget does not return an error if the property does
    # not exist, it simply does not output anything. So we do not need
    # to check for LaunchError here.
    args = '--strict "%s" "%s"' % (propname, url_or_dir)
    if rev:
        args = '-r %s %s' % (rev, args)
    out = launchsvn('propget %s' % args, split_lines=False)

    return dict_from_revlist_prop(out)

def get_merge_props(dir):
    """Extract the merged revisions."""
    return get_revlist_prop(dir, opts["prop"])

def get_block_props(dir):
    """Extract the blocked revisions."""
    return get_revlist_prop(dir, opts["block-prop"])

def get_blocked_revs(dir, source_pathid):
    p = get_block_props(dir)
    if p.has_key(source_pathid):
        return RevisionSet(p[source_pathid])
    return RevisionSet("")

def format_merge_props(props, sep=" "):
    """Formats the hash PROPS as a string suitable for use as a
    Subversion property value."""
    assert sep in ["\t", "\n", " "]   # must be a whitespace
    props = props.items()
    props.sort()
    L = []
    for h, r in props:
        L.append("%s:%s" % (h, r))
    return sep.join(L)

def _run_propset(dir, prop, value):
    """Set the property 'prop' of directory 'dir' to value 'value'. We go
    through a temporary file to not run into command line length limits."""
    try:
        fd, fname = tempfile.mkstemp()
        f = os.fdopen(fd, "wb")
    except AttributeError:
        # Fallback for Python <= 2.3 which does not have mkstemp (mktemp
        # suffers from race conditions. Not that we care...)
        fname = tempfile.mktemp()
        f = open(fname, "wb")

    try:
        f.write(value)
        f.close()
        report("property data written to temp file: %s" % value)
        svn_command('propset "%s" -F "%s" "%s"' % (prop, fname, dir))
    finally:
        os.remove(fname)

def set_props(dir, name, props):
    props = format_merge_props(props)
    if props:
        _run_propset(dir, name, props)
    else:
        # Check if NAME exists on DIR before trying to delete it.
        # As of 1.6 propdel no longer supports deleting a
        # non-existent property.
        out = launchsvn('propget "%s" "%s"' % (name, dir))
        if out:
            svn_command('propdel "%s" "%s"' % (name, dir))

def set_merge_props(dir, props):
    set_props(dir, opts["prop"], props)

def set_block_props(dir, props):
    set_props(dir, opts["block-prop"], props)

def set_blocked_revs(dir, source_pathid, revs):
    props = get_block_props(dir)
    if revs:
        props[source_pathid] = str(revs)
    elif props.has_key(source_pathid):
        del props[source_pathid]
    set_block_props(dir, props)

def is_url(url):
    """Check if url looks like a valid url."""
    return re.search(r"^[a-zA-Z][-+\.\w]*://[^\s]+$", url) is not None and url[:4] != 'uuid'

def check_url(url):
    """Similar to is_url, but actually invoke get_svninfo to find out"""
    return get_svninfo(url) != {}

def is_pathid(pathid):
    return isinstance(pathid, PathIdentifier)

def is_wc(dir):
    """Check if a directory is a working copy."""
    return os.path.isdir(os.path.join(dir, ".svn")) or \
           os.path.isdir(os.path.join(dir, "_svn"))

_cache_svninfo = {}
def get_svninfo(target):
    """Extract the subversion information for a target (through 'svn info').
    This function uses an internal cache to let clients query information
    many times."""
    if _cache_svninfo.has_key(target):
        return _cache_svninfo[target]
    info = {}
    for L in launchsvn('info "%s"' % target):
        L = L.strip()
        if not L:
            continue
        key, value = L.split(": ", 1)
        info[key] = value.strip()
    _cache_svninfo[target] = info
    return info

def target_to_url(target):
    """Convert working copy path or repos URL to a repos URL."""
    if is_wc(target):
        info = get_svninfo(target)
        return info["URL"]
    return target

_cache_reporoot = {}
def get_repo_root(target):
    """Compute the root repos URL given a working-copy path, or a URL."""
    # Try using "svn info WCDIR". This works only on SVN clients >= 1.3
    if not is_url(target):
        try:
            info = get_svninfo(target)
            root = info["Repository Root"]
            _cache_reporoot[root] = None
            return root
        except KeyError:
            pass
        url = target_to_url(target)
        assert url[-1] != '/'
    else:
        url = target

    # Go through the cache of the repository roots. This avoids extra
    # server round-trips if we are asking the root of different URLs
    # in the same repository (the cache in get_svninfo() cannot detect
    # that of course and would issue a remote command).
    assert is_url(url)
    for r in _cache_reporoot:
        if url.startswith(r):
            return r

    # Try using "svn info URL". This works only on SVN clients >= 1.2
    try:
        info = get_svninfo(url)
        # info may be {}, in which case we'll see KeyError here
        root = info["Repository Root"]
        _cache_reporoot[root] = None
        return root
    except (KeyError, LaunchError):
        pass

    # Constrained to older svn clients, we are stuck with this ugly
    # trial-and-error implementation. It could be made faster with a
    # binary search.
    while url:
        temp = os.path.dirname(url)
        try:
            launchsvn('proplist "%s"' % temp)
        except LaunchError:
            _cache_reporoot[url] = None
            return rstrip(url, "/")
        url = temp

    error("svn repos root of %s not found" % target)

class SvnLogParser:
    """
    Parse the "svn log", going through the XML output and using pulldom (which
    would even allow streaming the command output).
    """
    def __init__(self, xml):
        self._events = pulldom.parseString(xml)
    def __getitem__(self, idx):
        for event, node in self._events:
            if event == pulldom.START_ELEMENT and node.tagName == "logentry":
                self._events.expandNode(node)
                return self.SvnLogRevision(node)
        raise IndexError, "Could not find 'logentry' tag in xml"

    class SvnLogRevision:
        def __init__(self, xmlnode):
            self.n = xmlnode
        def revision(self):
            return int(self.n.getAttribute("revision"))
        def author(self):
            return self.n.getElementsByTagName("author")[0].firstChild.data
        def paths(self):
            return [self.SvnLogPath(n)
                    for n in  self.n.getElementsByTagName("path")]

        class SvnLogPath:
            def __init__(self, xmlnode):
                self.n = xmlnode
            def action(self):
                return self.n.getAttribute("action")
            def pathid(self):
                return self.n.firstChild.data
            def copyfrom_rev(self):
                try: return self.n.getAttribute("copyfrom-rev")
                except KeyError: return None
            def copyfrom_pathid(self):
                try: return self.n.getAttribute("copyfrom-path")
                except KeyError: return None

def get_copyfrom(target):
    """Get copyfrom info for a given target (it represents the
    repository-relative path from where it was branched). NOTE:
    repos root has no copyfrom info. In this case None is returned.

    Returns the:
        - source file or directory from which the copy was made
        - revision from which that source was copied
        - revision in which the copy was committed
    """
    repos_path = PathIdentifier.from_target(target).repo_relative_path
    for chg in SvnLogParser(launchsvn('log -v --xml --stop-on-copy "%s"'
                                      % target, split_lines=False)):
        for p in chg.paths():
            if p.action() == 'A' and p.pathid() == repos_path:
                # These values will be None if the corresponding elements are
                # not found in the log.
                return p.copyfrom_pathid(), p.copyfrom_rev(), chg.revision()
    return None,None,None

def get_latest_rev(url):
    """Get the latest revision of the repository of which URL is part."""
    try:
        info = get_svninfo(url)
        if not info.has_key("Revision"):
            error("Not a valid URL: %s" % url)
        return info["Revision"]
    except LaunchError:
        # Alternative method for latest revision checking (for svn < 1.2)
        report('checking latest revision of "%s"' % url)
        L = launchsvn('proplist --revprop -r HEAD "%s"' % opts["source-url"])[0]
        rev = re.search("revision (\d+)", L).group(1)
        report('latest revision of "%s" is %s' % (url, rev))
        return rev

def get_created_rev(url):
    """Lookup the revision at which the path identified by the
    provided URL was first created."""
    oldest_rev = -1
    report('determining oldest revision for URL "%s"' % url)
    ### TODO: Refactor this to use a modified RevisionLog class.
    lines = None
    cmd = "log -r1:HEAD --stop-on-copy -q " + url
    try:
        lines = launchsvn(cmd + " --limit=1")
    except LaunchError:
        # Assume that --limit isn't supported by the installed 'svn'.
        lines = launchsvn(cmd)
    if lines and len(lines) > 1:
        i = lines[1].find(" ")
        if i != -1:
            oldest_rev = int(lines[1][1:i])
    if oldest_rev == -1:
        error('unable to determine oldest revision for URL "%s"' % url)
    return oldest_rev

def get_commit_log(url, revnum):
    """Return the log message for a specific integer revision
    number."""
    out = launchsvn("log --incremental -r%d %s" % (revnum, url))
    return recode_stdout_to_file("".join(out[1:]))

def construct_merged_log_message(url, revnums):
    """Return a commit log message containing all the commit messages
    in the specified revisions at the given URL.  The separator used
    in this log message is determined by searching for the longest
    svnmerge separator existing in the commit log messages and
    extending it by one more separator.  This results in a new commit
    log message that is clearer in describing merges that contain
    other merges. Trailing newlines are removed from the embedded
    log messages."""
    messages = ['']
    longest_sep = ''
    for r in revnums.sorted():
        message = get_commit_log(url, r)
        if message:
            message = re.sub(r'(\r\n|\r|\n)', "\n", message)
            message = rstrip(message, "\n") + "\n"
            messages.append(prefix_lines(LOG_LINE_PREFIX, message))
            for match in LOG_SEPARATOR_RE.findall(message):
                sep = match[1]
                if len(sep) > len(longest_sep):
                    longest_sep = sep

    longest_sep += LOG_SEPARATOR + "\n"
    messages.append('')
    return longest_sep.join(messages)

def get_default_source(branch_target, branch_props):
    """Return the default source for branch_target (given its branch_props).
    Error out if there is ambiguity."""
    if not branch_props:
        error("no integration info available")

    props = branch_props.copy()
    pathid = PathIdentifier.from_target(branch_target)

    # To make bidirectional merges easier, find the target's
    # repository local path so it can be removed from the list of
    # possible integration sources.
    if props.has_key(pathid):
        del props[pathid]

    if len(props) > 1:
        err_msg = "multiple sources found. "
        err_msg += "Explicit source argument (-S/--source) required.\n"
        err_msg += "The merge sources available are:"
        for prop in props:
          err_msg += "\n  " + str(prop)
        error(err_msg)

    return props.keys()[0]

def should_find_reflected(branch_dir):
    should_find_reflected = opts["bidirectional"]

    # If the source has integration info for the target, set find_reflected
    # even if --bidirectional wasn't specified
    if not should_find_reflected:
        source_props = get_merge_props(opts["source-url"])
        should_find_reflected = source_props.has_key(PathIdentifier.from_target(branch_dir))

    return should_find_reflected

def analyze_revs(target_pathid, url, begin=1, end=None,
                 find_reflected=False):
    """For the source of the merges in the source URL being merged into
    target_pathid, analyze the revisions in the interval begin-end (which
    defaults to 1-HEAD), to find out which revisions are changes in
    the url, which are changes elsewhere (so-called 'phantom'
    revisions), optionally which are reflected changes (to avoid
    conflicts that can occur when doing bidirectional merging between
    branches), and which revisions initialize merge tracking against other
    branches.  Return a tuple of four RevisionSet's:
        (real_revs, phantom_revs, reflected_revs, initialized_revs).

    NOTE: To maximize speed, if "end" is not provided, the function is
    not able to find phantom revisions following the last real
    revision in the URL.
    """

    begin = str(begin)
    if end is None:
        end = "HEAD"
    else:
        end = str(end)
        if long(begin) > long(end):
            return RevisionSet(""), RevisionSet(""), \
                   RevisionSet(""), RevisionSet("")

    logs[url] = RevisionLog(url, begin, end, find_reflected)
    revs = RevisionSet(logs[url].revs)

    if end == "HEAD":
        # If end is not provided, we do not know which is the latest revision
        # in the repository. So return the phantom revision set only up to
        # the latest known revision.
        end = str(list(revs)[-1])

    phantom_revs = RevisionSet("%s-%s" % (begin, end)) - revs

    if find_reflected:
        reflected_revs = logs[url].merge_metadata().changed_revs(target_pathid)
        reflected_revs += logs[url].block_metadata().changed_revs(target_pathid)
    else:
        reflected_revs = []

    initialized_revs = RevisionSet(logs[url].merge_metadata().initialized_revs())
    reflected_revs = RevisionSet(reflected_revs)

    return revs, phantom_revs, reflected_revs, initialized_revs

def analyze_source_revs(branch_target, source_url, **kwargs):
    """For the given branch and source, extract the real and phantom
    source revisions."""
    branch_url = target_to_url(branch_target)
    branch_pathid = PathIdentifier.from_target(branch_target)

    # Extract the latest repository revision from the URL of the branch
    # directory (which is already cached at this point).
    end_rev = get_latest_rev(source_url)

    # Calculate the base of analysis. If there is a "1-XX" interval in the
    # merged_revs, we do not need to check those.
    base = 1
    r = opts["merged-revs"].normalized()
    if r and r[0][0] == 1:
        base = r[0][1] + 1

    # See if the user filtered the revision set. If so, we are not
    # interested in something outside that range.
    if opts["revision"]:
        revs = RevisionSet(opts["revision"]).sorted()
        if base < revs[0]:
            base = revs[0]
        if end_rev > revs[-1]:
            end_rev = revs[-1]

    return analyze_revs(branch_pathid, source_url, base, end_rev, **kwargs)

def minimal_merge_intervals(revs, phantom_revs):
    """Produce the smallest number of intervals suitable for merging. revs
    is the RevisionSet which we want to merge, and phantom_revs are phantom
    revisions which can be used to concatenate intervals, thus minimizing the
    number of operations."""
    revnums = revs.normalized()
    ret = []

    cur = revnums.pop()
    while revnums:
        next = revnums.pop()
        assert next[1] < cur[0]      # otherwise it is not ordered
        assert cur[0] - next[1] > 1  # otherwise it is not normalized
        for i in range(next[1]+1, cur[0]):
            if i not in phantom_revs:
                ret.append(cur)
                cur = next
                break
        else:
            cur = (next[0], cur[1])

    ret.append(cur)
    ret.reverse()
    return ret

def display_revisions(revs, display_style, revisions_msg, source_url):
    """Show REVS as dictated by DISPLAY_STYLE, either numerically, in
    log format, or as diffs.  When displaying revisions numerically,
    prefix output with REVISIONS_MSG when in verbose mode.  Otherwise,
    request logs or diffs using SOURCE_URL."""
    if display_style == "revisions":
        if revs:
            report(revisions_msg)
            print revs
    elif display_style == "logs":
        for start,end in revs.normalized():
            svn_command('log --incremental -v -r %d:%d %s' % \
                        (start, end, source_url))
    elif display_style in ("diffs", "summarize"):
        if display_style == 'summarize':
            summarize = '--summarize '
        else:
            summarize = ''

        for start, end in revs.normalized():
            print
            if start == end:
                print "%s: changes in revision %d follow" % (NAME, start)
            else:
                print "%s: changes in revisions %d-%d follow" % (NAME,
                                                                 start, end)
            print

            # Note: the starting revision number to 'svn diff' is
            # NOT inclusive so we have to subtract one from ${START}.
            svn_command("diff -r %d:%d %s %s" % (start - 1, end, summarize,
                                                 source_url))
    else:
        assert False, "unhandled display style: %s" % display_style

def action_init(target_dir, target_props):
    """Initialize for merges."""
    # Check that directory is ready for being modified
    check_dir_clean(target_dir)

    target_pathid = PathIdentifier.from_target(target_dir)
    source_pathid = opts['source-pathid']
    if source_pathid == target_pathid:
        error("cannot init integration source path '%s'\nIts path identifier does not "
              "differ from the path identifier of the current directory, '%s'."
              % (source_pathid, target_pathid))

    source_url = opts['source-url']

    # If the user hasn't specified the revisions to use, see if the
    # "source" is a copy from the current tree and if so, we can use
    # the version data obtained from it.
    revision_range = opts["revision"]
    if not revision_range:
        # If source was originally copied from target, and we are merging
        # changes from source to target (the copy target is the merge source,
        # and the copy source is the merge target), then we want to mark as
        # integrated up to the rev in which the copy was committed which
        # created the merge source:
        cf_source, cf_rev, copy_committed_in_rev = get_copyfrom(source_url)

        cf_pathid = None
        if cf_source:
            cf_url = get_repo_root(source_url) + cf_source
            if is_url(cf_url) and check_url(cf_url):
                cf_pathid = PathIdentifier.from_target(cf_url)

        if target_pathid == cf_pathid:
            report('the source "%s" was copied from "%s" in rev %s and committed in rev %s' %
                   (source_url, target_dir, cf_rev, copy_committed_in_rev))
            revision_range = "1-" + str(copy_committed_in_rev)

    if not revision_range:
        # If the reverse is true: copy source is the merge source, and
        # the copy target is the merge target, then we want to mark as
        # integrated up to the specific rev of the merge target from
        # which the merge source was copied.  (Longer discussion at:
        # http://subversion.tigris.org/issues/show_bug.cgi?id=2810  )
        cf_source, cf_rev, copy_committed_in_rev = get_copyfrom(target_dir)

        cf_pathid = None
        if cf_source:
            cf_url = get_repo_root(target_dir) + cf_source
            if is_url(cf_url) and check_url(cf_url):
                cf_pathid = PathIdentifier.from_target(cf_url)

        source_pathid = PathIdentifier.from_target(source_url)
        if source_pathid == cf_pathid:
            report('the target "%s" was copied the source "%s" in rev %s and committed in rev %s' %
                   (target_dir, source_url, cf_rev, copy_committed_in_rev))
            revision_range = "1-" + cf_rev

    # When neither the merge source nor target is a copy of the other, and
    # the user did not specify a revision range, then choose a default which is
    # the current revision; saying, in effect, "everything has been merged, so
    # mark as integrated up to the latest rev on source url).
    if not revision_range:
        revision_range = "1-" + get_latest_rev(source_url)

    revs = RevisionSet(revision_range)

    report('marking "%s" as already containing revisions "%s" of "%s"' %
           (target_dir, revs, source_url))

    revs = str(revs)
    # If the local svnmerge-integrated property already has an entry
    # for the source-pathid, simply error out.
    if not opts["force"] and target_props.has_key(source_pathid):
        error('Repository-relative path %s has already been initialized at %s\n'
              'Use --force to re-initialize' % (source_pathid, target_dir))
    # set the pathid's external_form based on the user's options
    source_pathid.external_form = source_pathid.format(opts['location-type'])

    revs = str(revs)
    target_props[source_pathid] = revs

    # Set property
    set_merge_props(target_dir, target_props)

    # Write out commit message if desired
    if opts["commit-file"]:
        f = open(opts["commit-file"], "w")
        print >>f, 'Initialized merge tracking via "%s" with revisions "%s" from ' \
            % (NAME, revs)
        print >>f, '%s' % source_url
        f.close()
        report('wrote commit message to "%s"' % opts["commit-file"])

def action_avail(branch_dir, branch_props):
    """Show commits available for merges."""
    source_revs, phantom_revs, reflected_revs, initialized_revs = \
               analyze_source_revs(branch_dir, opts["source-url"],
                                   find_reflected=
                                       should_find_reflected(branch_dir))
    report('skipping phantom revisions: %s' % phantom_revs)
    if reflected_revs:
        report('skipping reflected revisions: %s' % reflected_revs)
        report('skipping initialized revisions: %s' % initialized_revs)

    blocked_revs = get_blocked_revs(branch_dir, opts["source-pathid"])
    avail_revs = source_revs - opts["merged-revs"] - blocked_revs - \
                 reflected_revs - initialized_revs

    # Compose the set of revisions to show
    revs = RevisionSet("")
    report_msg = "revisions available to be merged are:"
    if "avail" in opts["avail-showwhat"]:
        revs |= avail_revs
    if "blocked" in opts["avail-showwhat"]:
        revs |= blocked_revs
        report_msg = "revisions blocked are:"

    # Limit to revisions specified by -r (if any)
    if opts["revision"]:
        revs = revs & RevisionSet(opts["revision"])

    display_revisions(revs, opts["avail-display"],
                      report_msg,
                      opts["source-url"])

def action_integrated(branch_dir, branch_props):
    """Show change sets already merged.  This set of revisions is
    calculated from taking svnmerge-integrated property from the
    branch, and subtracting any revision older than the branch
    creation revision."""
    # Extract the integration info for the branch_dir
    branch_props = get_merge_props(branch_dir)
    revs = merge_props_to_revision_set(branch_props, opts["source-pathid"])

    # Lookup the oldest revision on the branch path.
    oldest_src_rev = get_created_rev(opts["source-url"])

    # Subtract any revisions which pre-date the branch.
    report("subtracting revisions which pre-date the source URL (%d)" %
           oldest_src_rev)
    revs = revs - RevisionSet(range(1, oldest_src_rev))

    # Limit to revisions specified by -r (if any)
    if opts["revision"]:
        revs = revs & RevisionSet(opts["revision"])

    display_revisions(revs, opts["integrated-display"],
                      "revisions already integrated are:", opts["source-url"])

def action_merge(branch_dir, branch_props):
    """Record merge meta data, and do the actual merge (if not
    requested otherwise via --record-only)."""
    # Check branch directory is ready for being modified
    check_dir_clean(branch_dir)

    source_revs, phantom_revs, reflected_revs, initialized_revs = \
               analyze_source_revs(branch_dir, opts["source-url"],
                                   find_reflected=
                                       should_find_reflected(branch_dir))

    if opts["revision"]:
        revs = RevisionSet(opts["revision"])
    else:
        revs = source_revs

    blocked_revs = get_blocked_revs(branch_dir, opts["source-pathid"])
    merged_revs = opts["merged-revs"]

    # Show what we're doing
    if opts["verbose"]:  # just to avoid useless calculations
        if merged_revs & revs:
            report('"%s" already contains revisions %s' % (branch_dir,
                                                           merged_revs & revs))
        if phantom_revs:
            report('memorizing phantom revision(s): %s' % phantom_revs)
        if reflected_revs:
            report('memorizing reflected revision(s): %s' % reflected_revs)
        if blocked_revs & revs:
            report('skipping blocked revisions(s): %s' % (blocked_revs & revs))
        if initialized_revs:
            report('skipping initialized revision(s): %s' % initialized_revs)

    # Compute final merge set.
    revs = revs - merged_revs - blocked_revs - reflected_revs - \
           phantom_revs - initialized_revs
    if not revs:
        report('no revisions to merge, exiting')
        return

    # When manually marking revisions as merged, we only update the
    # integration meta data, and don't perform an actual merge.
    record_only = opts["record-only"]

    if record_only:
        report('recording merge of revision(s) %s from "%s"' %
               (revs, opts["source-url"]))
    else:
        report('merging in revision(s) %s from "%s"' %
               (revs, opts["source-url"]))

    # Do the merge(s). Note: the starting revision number to 'svn merge'
    # is NOT inclusive so we have to subtract one from start.
    # We try to keep the number of merge operations as low as possible,
    # because it is faster and reduces the number of conflicts.
    old_block_props = get_block_props(branch_dir)
    merge_metadata = logs[opts["source-url"]].merge_metadata()
    block_metadata = logs[opts["source-url"]].block_metadata()
    for start,end in minimal_merge_intervals(revs, phantom_revs):
        if not record_only:
            # Preset merge/blocked properties to the source value at
            # the start rev to avoid spurious property conflicts
            set_merge_props(branch_dir, merge_metadata.get(start - 1))
            set_block_props(branch_dir, block_metadata.get(start - 1))
            # Do the merge
            svn_command("merge --force -r %d:%d %s %s" % \
                        (start - 1, end, opts["source-url"], branch_dir))
            # TODO: to support graph merging, add logic to merge the property
            # meta-data manually

    # Update the set of merged revisions.
    merged_revs = merged_revs | revs | reflected_revs | phantom_revs | initialized_revs
    branch_props[opts["source-pathid"]] = str(merged_revs)
    set_merge_props(branch_dir, branch_props)
    # Reset the blocked revs
    set_block_props(branch_dir, old_block_props)

    # Write out commit message if desired
    if opts["commit-file"]:
        f = open(opts["commit-file"], "w")
        if record_only:
            print >>f, 'Recorded merge of revisions %s via %s from ' % \
                  (revs, NAME)
        else:
            print >>f, 'Merged revisions %s via %s from ' % \
                  (revs, NAME)
        print >>f, '%s' % opts["source-url"]
        if opts["commit-verbose"]:
            print >>f
            print >>f, construct_merged_log_message(opts["source-url"], revs),

        f.close()
        report('wrote commit message to "%s"' % opts["commit-file"])

def action_block(branch_dir, branch_props):
    """Block revisions."""
    # Check branch directory is ready for being modified
    check_dir_clean(branch_dir)

    source_revs, phantom_revs, reflected_revs, initialized_revs = \
               analyze_source_revs(branch_dir, opts["source-url"])
    revs_to_block = source_revs - opts["merged-revs"]

    # Limit to revisions specified by -r (if any)
    if opts["revision"]:
        revs_to_block = RevisionSet(opts["revision"]) & revs_to_block

    if not revs_to_block:
        error('no available revisions to block')

    # Change blocked information
    blocked_revs = get_blocked_revs(branch_dir, opts["source-pathid"])
    blocked_revs = blocked_revs | revs_to_block
    set_blocked_revs(branch_dir, opts["source-pathid"], blocked_revs)

    # Write out commit message if desired
    if opts["commit-file"]:
        f = open(opts["commit-file"], "w")
        print >>f, 'Blocked revisions %s via %s' % (revs_to_block, NAME)
        if opts["commit-verbose"]:
            print >>f
            print >>f, construct_merged_log_message(opts["source-url"],
                                                    revs_to_block),

        f.close()
        report('wrote commit message to "%s"' % opts["commit-file"])

def action_unblock(branch_dir, branch_props):
    """Unblock revisions."""
    # Check branch directory is ready for being modified
    check_dir_clean(branch_dir)

    blocked_revs = get_blocked_revs(branch_dir, opts["source-pathid"])
    revs_to_unblock = blocked_revs

    # Limit to revisions specified by -r (if any)
    if opts["revision"]:
        revs_to_unblock = revs_to_unblock & RevisionSet(opts["revision"])

    if not revs_to_unblock:
        error('no available revisions to unblock')

    # Change blocked information
    blocked_revs = blocked_revs - revs_to_unblock
    set_blocked_revs(branch_dir, opts["source-pathid"], blocked_revs)

    # Write out commit message if desired
    if opts["commit-file"]:
        f = open(opts["commit-file"], "w")
        print >>f, 'Unblocked revisions %s via %s' % (revs_to_unblock, NAME)
        if opts["commit-verbose"]:
            print >>f
            print >>f, construct_merged_log_message(opts["source-url"],
                                                    revs_to_unblock),
        f.close()
        report('wrote commit message to "%s"' % opts["commit-file"])

def action_rollback(branch_dir, branch_props):
    """Rollback previously integrated revisions."""

    # Make sure the revision arguments are present
    if not opts["revision"]:
        error("The '-r' option is mandatory for rollback")

    # Check branch directory is ready for being modified
    check_dir_clean(branch_dir)

    # Extract the integration info for the branch_dir
    branch_props = get_merge_props(branch_dir)
    # Get the list of all revisions already merged into this source-pathid.
    merged_revs = merge_props_to_revision_set(branch_props,
                                              opts["source-pathid"])

    # At which revision was the src created?
    oldest_src_rev = get_created_rev(opts["source-url"])
    src_pre_exist_range = RevisionSet("1-%d" % oldest_src_rev)

    # Limit to revisions specified by -r (if any)
    revs = merged_revs & RevisionSet(opts["revision"])

    # make sure there's some revision to rollback
    if not revs:
        report("Nothing to rollback in revision range r%s" % opts["revision"])
        return

    # If even one specified revision lies outside the lifetime of the
    # merge source, error out.
    if revs & src_pre_exist_range:
        err_str  = "Specified revision range falls out of the rollback range.\n"
        err_str += "%s was created at r%d" % (opts["source-pathid"],
                                              oldest_src_rev)
        error(err_str)

    record_only = opts["record-only"]

    if record_only:
        report('recording rollback of revision(s) %s from "%s"' %
               (revs, opts["source-url"]))
    else:
        report('rollback of revision(s) %s from "%s"' %
               (revs, opts["source-url"]))

    # Do the reverse merge(s). Note: the starting revision number
    # to 'svn merge' is NOT inclusive so we have to subtract one from start.
    # We try to keep the number of merge operations as low as possible,
    # because it is faster and reduces the number of conflicts.
    rollback_intervals = minimal_merge_intervals(revs, [])
    # rollback in the reverse order of merge
    rollback_intervals.reverse()
    for start, end in rollback_intervals:
        if not record_only:
            # Do the merge
            svn_command("merge --force -r %d:%d %s %s" % \
                        (end, start - 1, opts["source-url"], branch_dir))

    # Write out commit message if desired
    # calculate the phantom revs first
    if opts["commit-file"]:
        f = open(opts["commit-file"], "w")
        if record_only:
            print >>f, 'Recorded rollback of revisions %s via %s from ' % \
                  (revs , NAME)
        else:
            print >>f, 'Rolled back revisions %s via %s from ' % \
                  (revs , NAME)
        print >>f, '%s' % opts["source-url"]

        f.close()
        report('wrote commit message to "%s"' % opts["commit-file"])

    # Update the set of merged revisions.
    merged_revs = merged_revs - revs
    branch_props[opts["source-pathid"]] = str(merged_revs)
    set_merge_props(branch_dir, branch_props)

def action_uninit(branch_dir, branch_props):
    """Uninit SOURCE URL."""
    # Check branch directory is ready for being modified
    check_dir_clean(branch_dir)

    # If the source-pathid does not have an entry in the svnmerge-integrated
    # property, simply error out.
    if not branch_props.has_key(opts["source-pathid"]):
        error('Repository-relative path "%s" does not contain merge '
              'tracking information for "%s"' \
                % (opts["source-pathid"], branch_dir))

    del branch_props[opts["source-pathid"]]

    # Set merge property with the selected source deleted
    set_merge_props(branch_dir, branch_props)

    # Set blocked revisions for the selected source to None
    set_blocked_revs(branch_dir, opts["source-pathid"], None)

    # Write out commit message if desired
    if opts["commit-file"]:
        f = open(opts["commit-file"], "w")
        print >>f, 'Removed merge tracking for "%s" for ' % NAME
        print >>f, '%s' % opts["source-url"]
        f.close()
        report('wrote commit message to "%s"' % opts["commit-file"])

###############################################################################
# Command line parsing -- options and commands management
###############################################################################

class OptBase:
    def __init__(self, *args, **kwargs):
        self.help = kwargs["help"]
        del kwargs["help"]
        self.lflags = []
        self.sflags = []
        for a in args:
            if a.startswith("--"):   self.lflags.append(a)
            elif a.startswith("-"):  self.sflags.append(a)
            else:
                raise TypeError, "invalid flag name: %s" % a
        if kwargs.has_key("dest"):
            self.dest = kwargs["dest"]
            del kwargs["dest"]
        else:
            if not self.lflags:
                raise TypeError, "cannot deduce dest name without long options"
            self.dest = self.lflags[0][2:]
        if kwargs:
            raise TypeError, "invalid keyword arguments: %r" % kwargs.keys()
    def repr_flags(self):
        f = self.sflags + self.lflags
        r = f[0]
        for fl in f[1:]:
            r += " [%s]" % fl
        return r

class Option(OptBase):
    def __init__(self, *args, **kwargs):
        self.default = kwargs.setdefault("default", 0)
        del kwargs["default"]
        self.value = kwargs.setdefault("value", None)
        del kwargs["value"]
        OptBase.__init__(self, *args, **kwargs)
    def apply(self, state, value):
        assert value == ""
        if self.value is not None:
            state[self.dest] = self.value
        else:
            state[self.dest] += 1

class OptionArg(OptBase):
    def __init__(self, *args, **kwargs):
        self.default = kwargs["default"]
        del kwargs["default"]
        self.metavar = kwargs.setdefault("metavar", None)
        del kwargs["metavar"]
        OptBase.__init__(self, *args, **kwargs)

        if self.metavar is None:
            if self.dest is not None:
                self.metavar = self.dest.upper()
            else:
                self.metavar = "arg"
        if self.default:
            self.help += " (default: %s)" % self.default
    def apply(self, state, value):
        assert value is not None
        state[self.dest] = value
    def repr_flags(self):
        r = OptBase.repr_flags(self)
        return r + " " + self.metavar

class CommandOpts:
    class Cmd:
        def __init__(self, *args):
            self.name, self.func, self.usage, self.help, self.opts = args
        def short_help(self):
            return self.help.split(".")[0]
        def __str__(self):
            return self.name
        def __call__(self, *args, **kwargs):
            return self.func(*args, **kwargs)

    def __init__(self, global_opts, common_opts, command_table, version=None):
        self.progname = NAME
        self.version = version.replace("%prog", self.progname)
        self.cwidth = console_width() - 2
        self.ctable = command_table.copy()
        self.gopts = global_opts[:]
        self.copts = common_opts[:]
        self._add_builtins()
        for k in self.ctable.keys():
            cmd = self.Cmd(k, *self.ctable[k])
            opts = []
            for o in cmd.opts:
                if isinstance(o, types.StringType) or \
                   isinstance(o, types.UnicodeType):
                    o = self._find_common(o)
                opts.append(o)
            cmd.opts = opts
            self.ctable[k] = cmd

    def _add_builtins(self):
        self.gopts.append(
            Option("-h", "--help", help="show help for this command and exit"))
        if self.version is not None:
            self.gopts.append(
                Option("-V", "--version", help="show version info and exit"))
        self.ctable["help"] = (self._cmd_help,
            "help [COMMAND]",
            "Display help for a specific command. If COMMAND is omitted, "
            "display brief command description.",
            [])

    def _cmd_help(self, cmd=None, *args):
        if args:
            self.error("wrong number of arguments", "help")
        if cmd is not None:
            cmd = self._command(cmd)
            self.print_command_help(cmd)
        else:
            self.print_command_list()

    def _paragraph(self, text, width=78):
        chunks = re.split("\s+", text.strip())
        chunks.reverse()
        lines = []
        while chunks:
            L = chunks.pop()
            while chunks and len(L) + len(chunks[-1]) + 1 <= width:
                L += " " + chunks.pop()
            lines.append(L)
        return lines

    def _paragraphs(self, text, *args, **kwargs):
        pars = text.split("\n\n")
        lines = self._paragraph(pars[0], *args, **kwargs)
        for p in pars[1:]:
            lines.append("")
            lines.extend(self._paragraph(p, *args, **kwargs))
        return lines

    def _print_wrapped(self, text, indent=0):
        text = self._paragraphs(text, self.cwidth - indent)
        print text.pop(0)
        for t in text:
            print " " * indent + t

    def _find_common(self, fl):
        for o in self.copts:
            if fl in o.lflags+o.sflags:
                return o
        assert False, fl

    def _compute_flags(self, opts, check_conflicts=True):
        back = {}
        sfl = ""
        lfl = []
        for o in opts:
            sapp = lapp = ""
            if isinstance(o, OptionArg):
                sapp, lapp = ":", "="
            for s in o.sflags:
                if check_conflicts and back.has_key(s):
                    raise RuntimeError, "option conflict: %s" % s
                back[s] = o
                sfl += s[1:] + sapp
            for l in o.lflags:
                if check_conflicts and back.has_key(l):
                    raise RuntimeError, "option conflict: %s" % l
                back[l] = o
                lfl.append(l[2:] + lapp)
        return sfl, lfl, back

    def _extract_command(self, args):
        """
        Try to extract the command name from the argument list. This is
        non-trivial because we want to allow command-specific options even
        before the command itself.
        """
        opts = self.gopts[:]
        for cmd in self.ctable.values():
            opts.extend(cmd.opts)
        sfl, lfl, _ = self._compute_flags(opts, check_conflicts=False)

        lopts,largs = getopt.getopt(args, sfl, lfl)
        if not largs:
            return None
        return self._command(largs[0])

    def _fancy_getopt(self, args, opts, state=None):
        if state is None:
            state= {}
        for o in opts:
            if not state.has_key(o.dest):
                state[o.dest] = o.default

        sfl, lfl, back = self._compute_flags(opts)
        try:
            lopts,args = getopt.gnu_getopt(args, sfl, lfl)
        except AttributeError:
            # Before Python 2.3, there was no gnu_getopt support.
            # So we can't parse intermixed positional arguments
            # and options.
            lopts,args = getopt.getopt(args, sfl, lfl)

        for o,v in lopts:
            back[o].apply(state, v)
        return state, args

    def _command(self, cmd):
        if not self.ctable.has_key(cmd):
            self.error("unknown command: '%s'" % cmd)
        return self.ctable[cmd]

    def parse(self, args):
        if not args:
            self.print_small_help()
            sys.exit(0)

        cmd = None
        try:
            cmd = self._extract_command(args)
            opts = self.gopts[:]
            if cmd:
                opts.extend(cmd.opts)
                args.remove(cmd.name)
            state, args = self._fancy_getopt(args, opts)
        except getopt.GetoptError, e:
            self.error(e, cmd)

        # Handle builtins
        if self.version is not None and state["version"]:
            self.print_version()
            sys.exit(0)
        if state["help"]: # special case for --help
            if cmd:
                self.print_command_help(cmd)
                sys.exit(0)
            cmd = self.ctable["help"]
        else:
            if cmd is None:
                self.error("command argument required")
        if str(cmd) == "help":
            cmd(*args)
            sys.exit(0)
        return cmd, args, state

    def error(self, s, cmd=None):
        print >>sys.stderr, "%s: %s" % (self.progname, s)
        if cmd is not None:
            self.print_command_help(cmd)
        else:
            self.print_small_help()
        sys.exit(1)
    def print_small_help(self):
        print "Type '%s help' for usage" % self.progname
    def print_usage_line(self):
        print "usage: %s <subcommand> [options...] [args...]\n" % self.progname
    def print_command_list(self):
        print "Available commands (use '%s help COMMAND' for more details):\n" \
              % self.progname
        cmds = self.ctable.keys()
        cmds.sort()
        indent = max(map(len, cmds))
        for c in cmds:
            h = self.ctable[c].short_help()
            print "  %-*s   " % (indent, c),
            self._print_wrapped(h, indent+6)
    def print_command_help(self, cmd):
        cmd = self.ctable[str(cmd)]
        print 'usage: %s %s\n' % (self.progname, cmd.usage)
        self._print_wrapped(cmd.help)
        def print_opts(opts, self=self):
            if not opts: return
            flags = [o.repr_flags() for o in opts]
            indent = max(map(len, flags))
            for f,o in zip(flags, opts):
                print "  %-*s :" % (indent, f),
                self._print_wrapped(o.help, indent+5)
        print '\nCommand options:'
        print_opts(cmd.opts)
        print '\nGlobal options:'
        print_opts(self.gopts)

    def print_version(self):
        print self.version

###############################################################################
# Options and Commands description
###############################################################################

global_opts = [
    Option("-F", "--force",
           help="force operation even if the working copy is not clean, or "
                "there are pending updates"),
    Option("-n", "--dry-run",
           help="don't actually change anything, just pretend; "
                "implies --show-changes"),
    Option("-s", "--show-changes",
           help="show subversion commands that make changes"),
    Option("-v", "--verbose",
           help="verbose mode: output more information about progress"),
    OptionArg("-u", "--username",
              default=None,
              help="invoke subversion commands with the supplied username"),
    OptionArg("-p", "--password",
              default=None,
              help="invoke subversion commands with the supplied password"),
    OptionArg("-c", "--config-dir", metavar="DIR",
              default=None,
              help="cause subversion commands to consult runtime config directory DIR"),
]

common_opts = [
    Option("-b", "--bidirectional",
           value=True,
           default=False,
           help="remove reflected and initialized revisions from merge candidates.  "
                "Not required but may be specified to speed things up slightly"),
    OptionArg("-f", "--commit-file", metavar="FILE",
              default="svnmerge-commit-message.txt",
              help="set the name of the file where the suggested log message "
                   "is written to"),
    Option("-M", "--record-only",
           value=True,
           default=False,
           help="do not perform an actual merge of the changes, yet record "
                "that a merge happened"),
    OptionArg("-r", "--revision",
              metavar="REVLIST",
              default="",
              help="specify a revision list, consisting of revision numbers "
                   'and ranges separated by commas, e.g., "534,537-539,540"'),
    OptionArg("-S", "--source", "--head",
              default=None,
              help="specify a merge source for this branch.  It can be either "
                   "a working directory path, a full URL, or an unambiguous "
                   "substring of one of the locations for which merge tracking was "
                   "already initialized.  Needed only to disambiguate in case of "
                   "multiple merge sources"),
]

command_table = {
    "init": (action_init,
    "init [OPTION...] [SOURCE]",
    """Initialize merge tracking from SOURCE on the current working
    directory.

    If SOURCE is specified, all the revisions in SOURCE are marked as already
    merged; if this is not correct, you can use --revision to specify the
    exact list of already-merged revisions.

    If SOURCE is omitted, then it is computed from the "svn cp" history of the
    current working directory (searching back for the branch point); in this
    case, %s assumes that no revision has been integrated yet since
    the branch point (unless you teach it with --revision).""" % NAME,
    [
        "-f", "-r", # import common opts
        OptionArg("-L", "--location-type",
               dest="location-type",
               default="path",
               help="Use this type of location identifier in the new " +
                    "Subversion properties; 'uuid', 'url', or 'path' " +
                    "(default)"),
    ]),

    "avail": (action_avail,
    "avail [OPTION...] [PATH]",
    """Show unmerged revisions available for PATH as a revision list.
    If --revision is given, the revisions shown will be limited to those
    also specified in the option.

    When svnmerge is used to bidirectionally merge changes between a
    branch and its source, it is necessary to not merge the same changes
    forth and back: e.g., if you committed a merge of a certain
    revision of the branch into the source, you do not want that commit
    to appear as available to merged into the branch (as the code
    originated in the branch itself!).  svnmerge will automatically
    exclude these so-called "reflected" revisions.""",
    [
        Option("-A", "--all",
               dest="avail-showwhat",
               value=["blocked", "avail"],
               default=["avail"],
               help="show both available and blocked revisions (aka ignore "
                    "blocked revisions)"),
        "-b",
        Option("-B", "--blocked",
               dest="avail-showwhat",
               value=["blocked"],
               help="show the blocked revision list (see '%s block')" % NAME),
        Option("-d", "--diff",
               dest="avail-display",
               value="diffs",
               default="revisions",
               help="show corresponding diff instead of revision list"),
        Option("--summarize",
               dest="avail-display",
               value="summarize",
               help="show summarized diff instead of revision list"),
        Option("-l", "--log",
               dest="avail-display",
               value="logs",
               help="show corresponding log history instead of revision list"),
        "-r",
        "-S",
    ]),

    "integrated": (action_integrated,
    "integrated [OPTION...] [PATH]",
    """Show merged revisions available for PATH as a revision list.
    If --revision is given, the revisions shown will be limited to
    those also specified in the option.""",
    [
        Option("-d", "--diff",
               dest="integrated-display",
               value="diffs",
               default="revisions",
               help="show corresponding diff instead of revision list"),
        Option("-l", "--log",
               dest="integrated-display",
               value="logs",
               help="show corresponding log history instead of revision list"),
        "-r",
        "-S",
    ]),

    "rollback": (action_rollback,
    "rollback [OPTION...] [PATH]",
    """Rollback previously merged in revisions from PATH.  The
    --revision option is mandatory, and specifies which revisions
    will be rolled back.  Only the previously integrated merges
    will be rolled back.

    When manually rolling back changes, --record-only can be used to
    instruct %s that a manual rollback of a certain revision
    already happened, so that it can record it and offer that
    revision for merge henceforth.""" % (NAME),
    [
        "-f", "-r", "-S", "-M", # import common opts
    ]),

    "merge": (action_merge,
    "merge [OPTION...] [PATH]",
    """Merge in revisions into PATH from its source. If --revision is omitted,
    all the available revisions will be merged. In any case, already merged-in
    revisions will NOT be merged again.

    When svnmerge is used to bidirectionally merge changes between a
    branch and its source, it is necessary to not merge the same changes
    forth and back: e.g., if you committed a merge of a certain
    revision of the branch into the source, you do not want that commit
    to appear as available to merged into the branch (as the code
    originated in the branch itself!).  svnmerge will automatically
    exclude these so-called "reflected" revisions.

    When manually merging changes across branches, --record-only can
    be used to instruct %s that a manual merge of a certain revision
    already happened, so that it can record it and not offer that
    revision for merge anymore.  Conversely, when there are revisions
    which should not be merged, use '%s block'.""" % (NAME, NAME),
    [
        "-b", "-f", "-r", "-S", "-M", # import common opts
    ]),

    "block": (action_block,
    "block [OPTION...] [PATH]",
    """Block revisions within PATH so that they disappear from the available
    list. This is useful to hide revisions which will not be integrated.
    If --revision is omitted, it defaults to all the available revisions.

    Do not use this option to hide revisions that were manually merged
    into the branch.  Instead, use '%s merge --record-only', which
    records that a merge happened (as opposed to a merge which should
    not happen).""" % NAME,
    [
        "-f", "-r", "-S", # import common opts
    ]),

    "unblock": (action_unblock,
    "unblock [OPTION...] [PATH]",
    """Revert the effect of '%s block'. If --revision is omitted, all the
    blocked revisions are unblocked""" % NAME,
    [
        "-f", "-r", "-S", # import common opts
    ]),

    "uninit": (action_uninit,
    "uninit [OPTION...] [PATH]",
    """Remove merge tracking information from PATH. It cleans any kind of merge
    tracking information (including the list of blocked revisions). If there
    are multiple sources, use --source to indicate which source you want to
    forget about.""",
    [
        "-f", "-S", # import common opts
    ]),
}


def main(args):
    global opts

    # Initialize default options
    opts = default_opts.copy()
    logs.clear()

    optsparser = CommandOpts(global_opts, common_opts, command_table,
                             version="%%prog r%s\n  modified: %s\n\n"
                                     "Copyright (C) 2004,2005 Awarix Inc.\n"
                                     "Copyright (C) 2005, Giovanni Bajo"
                                     % (__revision__, __date__))

    cmd, args, state = optsparser.parse(args)
    opts.update(state)

    source = opts.get("source", None)
    branch_dir = "."

    if str(cmd) == "init":
        if len(args) == 1:
            source = args[0]
        elif len(args) > 1:
            optsparser.error("wrong number of parameters", cmd)
    elif str(cmd) in command_table.keys():
        if len(args) == 1:
            branch_dir = args[0]
        elif len(args) > 1:
            optsparser.error("wrong number of parameters", cmd)
    else:
        assert False, "command not handled: %s" % cmd

    # Validate branch_dir
    if not is_wc(branch_dir):
        if str(cmd) == "avail":
            info = None
            # it should be noted here that svn info does not error exit
            # if an invalid target is specified to it (as is
            # intuitive). so the try, except code is not absolutely
            # necessary. but, I retain it to indicate the intuitive
            # handling.
            try:
                info = get_svninfo(branch_dir)
            except LaunchError:
                pass
            # test that we definitely targeted a subversion directory,
            # mirroring the purpose of the earlier is_wc() call
            if info is None or not info.has_key("Node Kind") or info["Node Kind"] != "directory":
                error('"%s" is neither a valid URL, nor a working directory' % branch_dir)
        else:
            error('"%s" is not a subversion working directory' % branch_dir)

    # give out some hints as to potential pathids
    PathIdentifier.hint(branch_dir)
    if source: PathIdentifier.hint(source)

    # Extract the integration info for the branch_dir
    branch_props = get_merge_props(branch_dir)

    # Calculate source_url and source_path
    report("calculate source path for the branch")
    if not source:
        if str(cmd) == "init":
            cf_source, cf_rev, copy_committed_in_rev = get_copyfrom(branch_dir)
            if not cf_source:
                error('no copyfrom info available. '
                      'Explicit source argument (-S/--source) required.')
            opts["source-url"] = get_repo_root(branch_dir) + cf_source
            opts["source-pathid"] = PathIdentifier.from_target(opts["source-url"])

            if not opts["revision"]:
                opts["revision"] = "1-" + cf_rev
        else:
            opts["source-pathid"] = get_default_source(branch_dir, branch_props)
            opts["source-url"] = opts["source-pathid"].get_url()

        assert is_pathid(opts["source-pathid"])
        assert is_url(opts["source-url"])
    else:
        # The source was given as a command line argument and is stored in
        # SOURCE.  Ensure that the specified source does not end in a /,
        # otherwise it's easy to have the same source path listed more
        # than once in the integrated version properties, with and without
        # trailing /'s.
        source = rstrip(source, "/")
        if not is_wc(source) and not is_url(source):
            # Check if it is a substring of a pathid recorded
            # within the branch properties.
            found = []
            for pathid in branch_props.keys():
                if pathid.match_substring(source):
                    found.append(pathid)
            if len(found) == 1:
                # (assumes pathid is a repository-relative-path)
                source_pathid = found[0]
                source = source_pathid.get_url()
            else:
                error('"%s" is neither a valid URL, nor an unambiguous '
                      'substring of a repository path, nor a working directory'
                      % source)
        else:
            source_pathid = PathIdentifier.from_target(source)

        source_pathid = PathIdentifier.from_target(source)
        if str(cmd) == "init" and \
               source_pathid == PathIdentifier.from_target("."):
            error("cannot init integration source path '%s'\n"
                  "Its repository-relative path must differ from the "
                  "repository-relative path of the current directory."
                  % source_pathid)
        opts["source-pathid"] = source_pathid
        opts["source-url"] = target_to_url(source)

    # Sanity check source_url
    assert is_url(opts["source-url"])
    # SVN does not support non-normalized URL (and we should not
    # have created them)
    assert opts["source-url"].find("/..") < 0

    report('source is "%s"' % opts["source-url"])

    # Get previously merged revisions (except when command is init)
    if str(cmd) != "init":
        opts["merged-revs"] = merge_props_to_revision_set(branch_props,
                                                          opts["source-pathid"])

    # Perform the action
    cmd(branch_dir, branch_props)


if __name__ == "__main__":
    try:
        main(sys.argv[1:])
    except LaunchError, (ret, cmd, out):
        err_msg = "command execution failed (exit code: %d)\n" % ret
        err_msg += cmd + "\n"
        err_msg += "".join(out)
        error(err_msg)
    except KeyboardInterrupt:
        # Avoid traceback on CTRL+C
        print "aborted by user"
        sys.exit(1)
