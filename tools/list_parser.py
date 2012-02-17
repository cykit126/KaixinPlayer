# coding: utf-8

import sys

def parse_item(a_line):
    segments = a_line.split('|')
    name = segments[2]
    uri = segments[5]
    return name,uri

if __name__ == '__main__':
    if (len(sys.argv) < 2):
        print("usage: python list_converter.py file")
        exit(1)

    print "<channels>"
        
    file_path = sys.argv[1]
    with open(file_path) as f:
        for line in f:
            name,uri = parse_item(line)
            print "    <item>"
            print "        <uri type=\"String\">%s</uri>" % uri
            print "        <name type=\"com.kaixindev.core.I18NString\">"
            print "            <en type=\"String\">%s</en>" % name
            print "        </name>"
            print "    </item>"

    print "</channels>"
