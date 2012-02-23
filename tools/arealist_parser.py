# coding: utf-8

import sys
import codecs

def parse_item(a_line):
    segments = a_line.split('|')
    area_id = segments[0]
    name_zh = segments[1]
    name_en = segments[2]
    return area_id,name_zh,name_en

if __name__ == '__main__':
    if (len(sys.argv) < 3):
        print("usage: python list_converter.py input_file output_file")
        exit(1)
        
    content = "<areas type=\"com.kaixindev.kxplayer.Area[]\">\n"
        
    file_path = sys.argv[1]
    with codecs.open(file_path, "r", "utf-8") as f:
        for line in f:
            area_id,name_zh,name_en = parse_item(line)
            content = content + "    <item>\n"
            content = content + "        <id type=\"String\">%s</id>\n" % area_id
            content = content + "        <name type=\"com.kaixindev.core.I18NString\">\n"
            content = content + "            <en type=\"String\">%s</en>\n" % name_en
            content = content + "            <zh type=\"String\">%s</zh>\n" % name_zh
            content = content + "        </name>\n"
            content = content + "    </item>\n"

    content = content + "</areas>\n"

    out_path = sys.argv[2]
    with codecs.open(out_path, "w", "utf-8") as f:
        f.write(content)


