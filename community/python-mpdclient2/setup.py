#!/usr/bin/env python

# $Id: setup.py,v 1.1 2006/01/13 00:46:12 willysilly Exp $

from distutils.core import setup

setup(
  name="py-libmpdclient",
  version="0.10.0",
  description="Python client library for mpd (Music Player Daemon)",
  author="Nick Welch",
  author_email="mack at incise dot org",
  url="http://www.musicpd.org/?page=python_module",
  py_modules=['mpdclient2'],
  license="GNU Lesser General Public License"
)

