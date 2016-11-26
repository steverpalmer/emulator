# site_init.py
# Copyright 2016 Steve Palmer

import subprocess

#try:
#    SetOption('num_jobs', int(subprocess.check_output('nproc')))
#except:
#    pass

common_env = Environment()

common_env['BUILDERS']['uxf2png'] = Builder(action='umlet -action=convert -format=png -filename=$SOURCE',
                                            suffix='uxf.png', src_suffix='uxf')

common_env['BUILDERS']['rng2rnc'] = Builder(action='trang -I rng -O rnc $SOURCE $TARGET 2>/dev/null',
                                            suffix='rnc', src_suffix='rng')
common_env['BUILDERS']['rnc2rng'] = Builder(action='trang -I rnc -O rng $SOURCE $TARGET 2>/dev/null',
                                            suffix='rng', src_suffix='rnc')
common_env['BUILDERS']['rng2xsd'] = Builder(action='trang -I rng -O xsd $SOURCE $TARGET 2>/dev/null',
                                            suffix='xsd', src_suffix='rng')
common_env['BUILDERS']['rnc2xsd'] = Builder(action='trang -I rnc -O xsd $SOURCE $TARGET 2>/dev/null',
                                            suffix='xsd', src_suffix='rnc')

common_env['BUILDERS']['jing'] = Builder(action='jing $SOURCES >$TARGET 2>/dev/null')

# common_env['BUILDERS']['robot'] = Builder(action='robot $SOURCES', src_suffix='robot')

Export('common_env')
