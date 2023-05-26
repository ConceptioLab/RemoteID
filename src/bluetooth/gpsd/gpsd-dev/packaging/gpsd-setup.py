"""
gpsd-setup.py - Build gpsd Python modules for additional Pythons.

This file is Copyright 2020 by the GPSD project
SPDX-License-Identifier: BSD-2-clause
"""
# This code runs compatibly under Python 2 and 3.x for x >= 2.
# Preserve this property!
try:
    from setuptools import setup
except ImportError:  # No setuptools in Python 2
    from distutils.core import setup

setup(
    name='gps',
    version='3.24.1~dev',
    description='gpsd Python modules for additional Pythons',
    long_description='''# README

the gps module is a translator between gpsd and its' clients.
gps communicates on a port (2947) requesting information.  The
communication is JSON formatted. gps takes this information
from gpsd and translates it a pythonic interface easier to
understand for clients.

This module is for additional Pythons.''',
    long_description_content_type='text/markdown',
    url='https://gpsd.io/',
    author='the GPSD project',
    author_email='gpsd-dev@lists.nongnu.org',
    license='BSD',
    packages=['gps'],
    project_urls={
        'download': 'http://download-mirror.savannah.gnu.org/releases/gpsd/',
        'irc':      'irc://chat.freenode.net/#gpsd',
        'issues':   'https://gitlab.com/gpsd/gpsd/-/issues',
        'lists':    'https://lists.nongnu.org/mailman/listinfo/',
        'project':  'https://gitlab.com/gpsd/gpsd',
        'support':  'https://gpsd.io/SUPPORT.html',
        'tiplink':  'https://www.patreon.com/esr',
    },
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'License :: OSI Approved :: BSD License',
        'Operating System :: POSIX',
        'Programming Language :: Python',
    ],
)
