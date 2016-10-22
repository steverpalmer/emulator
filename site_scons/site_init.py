import subprocess

try:
    SetOption('num_jobs', int(subprocess.check_output('nproc')))
except:
    pass

uxf2png = Builder(action='umlet -action=convert -format=png -filename=$SOURCE',
                  suffix='uxf.png', src_suffix='uxf')

rng2rnc = Builder(action='trang -I rng -O rnc $SOURCE $TARGET',
                  suffix='rnc', src_suffix='rng')
rng2xsd = Builder(action='trang -I rng -O xsd $SOURCE $TARGET',
                  suffix='xsd', src_suffix='rng')
