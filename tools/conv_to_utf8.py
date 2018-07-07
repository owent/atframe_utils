#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys;
import getopt, chardet;

""" run as a executable """
if __name__ == "__main__":
    def print_help_msg():
        print('usage: ' + sys.argv[0] + ' [options...] [file paths...]')
        print('options:')
        print('-h, --help                               help messages')
        print('-v, --version                            show version and exit')
        print('-b, --with-bom                           utf-8 with bom')


    enable_bom = False
    opts, left_args = getopt.getopt(sys.argv[1:], 'bhv', [
        'help',
        'version',
        'with-bom'
    ])

    stats_conv_num = 0
    stats_fix_bom_num = 0
    stats_skip_num = 0
    stats_failed_num = 0
    
    for opt_key, opt_val in opts:
        if opt_key in ('-b', '--with-bom'):
            enable_bom = True
        elif opt_key in ('-h', '--help'):
            print_help_msg()
            exit(0)
        elif opt_key in ('-v', '--version'):
            print('1.0.0.0')
            exit(0)

    if len(left_args) > 0:
        for file_path in left_args:
            try: 
                f = open(file_path, 'rb')
            except Exception:
                stats_failed_num = stats_failed_num + 1
                print('open file ' + file_path + ' failed')
                continue
            
            file_buffers = f.read()
            f.close()
            detected_info = chardet.detect(file_buffers)
            if detected_info['encoding'] is not None:
                if detected_info['encoding'].lower() == 'utf-8':
                    if enable_bom and file_buffers[0:3] == "\xEF\xBB\xBF":
                        stats_skip_num = stats_skip_num + 1
                        print(file_path + ' is already utf-8 with bom, skiped')
                    elif not enable_bom and file_buffers[0:3] != "\xEF\xBB\xBF":
                        stats_skip_num = stats_skip_num + 1
                        print(file_path + ' is already utf-8 without bom, skiped.')
                    elif enable_bom:
                        f = open(file_path, 'wb')
                        f.write("\xEF\xBB\xBF")
                        f.write(file_buffers)
                        f.close()
                        print(file_path + ' add utf-8 bom done')
                        stats_fix_bom_num = stats_fix_bom_num + 1
                    else:
                        f = open(file_path, 'wb')
                        f.write(file_buffers[3:])
                        f.truncate()
                        f.close()
                        print(file_path + ' remove utf-8 bom done')
                        stats_fix_bom_num = stats_fix_bom_num + 1
                else:
                    try: 
                        if file_buffers[0:3] == "\xEF\xBB\xBF":
                            new_buffers = file_buffers[3:].decode(detected_info['encoding']).encode('utf-8')
                        else:
                            new_buffers = file_buffers.decode(detected_info['encoding']).encode('utf-8')
                        f = open(file_path, 'wb')
                        suffix = 'witout bom'
                        if enable_bom:
                            f.write("\xEF\xBB\xBF")
                            suffix = 'with bom'
                        
                        
                        f.write(new_buffers)
                        f.close()
                        
                        print(file_path + ' convert from ' + detected_info['encoding'] + ' to utf-8 ' + suffix + ' success.')
                        stats_conv_num = stats_conv_num + 1
                    except Exception:
                        stats_failed_num = stats_failed_num + 1
                        print(file_path + ' convert from ' + detected_info['encoding'] + ' to utf-8 ' + suffix + ' failed.')
            else:
                print(file_path + ' can not detect encoding')
                stats_skip_num = stats_skip_num + 1
    print('')
    print('all jobs done. convert number: {0}, fix bom number: {1}, skip number: {2}, failed number: {3}'.format(stats_conv_num, stats_fix_bom_num, stats_skip_num, stats_failed_num))
