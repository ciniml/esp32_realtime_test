#!/usr/bin/env python3

import serial
import io
import re
import os
import datetime

def main():
    com_port = os.environ['ESPPORT']
    com_baud = 115200 #int(os.environ['ESPBAUD'])
    print('Using serial port {0}:{1}'.format(com_port, com_baud))

    begin_pattern = re.compile(r'DUMP BEGIN (\d+):')
    end_pattern = re.compile(r'DUMP END:')

    com = serial.Serial(port=com_port, baudrate=com_baud, timeout=None)

    now = datetime.datetime.now()
    main_log_name = 'log_main_{0:%Y%m%d%H%M%S}.log'.format(now)
    print('Log file: ' + main_log_name)
    
    with open(main_log_name, 'w') as main_log: 
        dump_file = None
        for line in iter(com.readline, ''):
            line = line.decode(encoding='utf-8')
            main_log.write(line)
            
            line = line.rstrip()
            if dump_file is None:
                match = begin_pattern.match(line)
                if match is not None:
                    timestamp = match.group(1)
                    filename = 'log_{0}.csv'.format(timestamp)
                    print('Dump begin {0}'.format(filename))
                    dump_file = open(filename, 'w')
                else:
                    print(line)
            else:
                match = end_pattern.match(line)
                if match is not None:
                    dump_file.close()
                    dump_file = None
                    print('Dump end')
                else:
                    dump_file.write(line)
                    dump_file.write('\n')

main()