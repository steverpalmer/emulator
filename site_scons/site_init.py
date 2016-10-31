import subprocess

try:
    SetOption('num_jobs', int(subprocess.check_output('nproc')))
except:
    pass

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

Export('common_env')
