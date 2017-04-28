#! python3
# -*- coding: utf-8 -*-

from sys import argv
from time import time, localtime, strftime

t = int(time())

print(argv)

info_file = '../build_info.hpp'

def split(line):
    delim = '= '
    res = line.split(delim, 1)
    res[0] += delim
    res[1] = res[1].rsplit(';', 1)[0]
    return res

def parse_toolchain_version(toolchain_settings):
        prefix = 'XMEGAA_DFP\\'
        start = toolchain_settings.find(prefix)
        if start == -1:
                return '<Unknown>'
        start += len(prefix)
        return toolchain_settings[start:toolchain_settings.find('\\', start)]

value_modifier = [
    lambda x: str(int(x) + 1),
    lambda x: str(t),
    lambda x: '"' + strftime('%d.%m.%Y %H:%M:%S', localtime(t)) + '"',
    lambda x: '"' + argv[1] + '"',
    lambda x: '"' + argv[2] + '"',
    lambda x: '"' + parse_toolchain_version(argv[3]) + '"'   ]
log_text = ['Build ', '', ': ', ' <', '> by ', ' (toolchain {})']
log_info = ''
f = open(info_file, 'r')
build_info = f.readlines()
f.close()
value_index = build_info.index('/***** VALUES *****/\n') + 1
for i in range(5):
    (variable, value) = split(build_info[value_index + i])
    new_value = value_modifier[i](value)
    build_info[value_index + i] = variable + new_value + ';\n'
    if new_value[0] == '"':
        new_value = new_value[1:-1]
    if i != 1:
                if '{' in log_text[i]:
                        log_info += log_text[i].format(new_value)
                else:
                        log_info += log_text[i] + new_value
f = open(info_file, 'w')
f.writelines(build_info);
f.close()
print(log_info)
