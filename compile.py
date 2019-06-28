#!/usr/bin/env python

import sys
import os
import re

TARGET_HEADER_FILE = "build/includes/cradle.hpp" 
PRAGMA_ONCE_MACRO = "#pragma once"
INCLUDE_SEARCH_PATH = "includes"
INCLUDE_REGEX_STRING = r'#include\s+["<]([^"\n]*)[">]'

class HeaderFile:
    def __init__(self, path):
        self.path = path
        self.processed = False

    def get_path(self):
        return self.path

    def mark_processed(self):
        self.processed = True

    def is_processed(self):
        return self.processed

def create_context():
    headers = {}

    for dirname, subdirs, files in os.walk(INCLUDE_SEARCH_PATH):
        for filename in files:
            header_file_path = os.path.join(dirname, filename)
            key = header_file_path[len(INCLUDE_SEARCH_PATH + "/"):]

            headers[key] = HeaderFile(header_file_path)

    return headers

def build_helper(target, context, headerFile):
    """Process file at path source as a header. Resolves its includes against context and outputs to target."""

    # Don't reprocess a file that has been processed.
    if headerFile.processed:
        return
    else:
        headerFile.mark_processed()

    f = open(headerFile.get_path())
    L = f.readlines()
    f.close()

    m = re.compile(INCLUDE_REGEX_STRING)

    for line in L:
        include = m.match(line)
        if include and include.group(1) in context:
            # Recursively handle includes we know about.
            build_helper(target, context, context[include.group(1)])
        elif line.startswith(PRAGMA_ONCE_MACRO):
            pass
        else:
            target.write(line)

def build(context, targetfile):
    print "Writing unified header to:", targetfile

    # Make the parent directory.
    target_parent_dir = os.path.dirname(targetfile)
    if not os.path.exists(target_parent_dir):
        os.makedirs(target_parent_dir)

    target = open(targetfile, "w")

    for key in context:
        build_helper(target, context, context[key])

    target.close()

def main(args):
    context = create_context()

    print "Creating unified header from:"
    for header in sorted(context.keys()):
	print '\t', header
    print

    build(context, TARGET_HEADER_FILE)
            
if __name__=="__main__":
    main(sys.argv)
