import subprocess

def add_library(env, lib):
    def convert(cmd, prefix):
        return [ (opt[2:] if opt.startswith(prefix) else opt) for opt in cmd.split() ]
    env.AppendUnique( CFLAGS    = subprocess.check_output(['pkg-config', '--cflags-only-other', lib]).split()
                    , CPPPATH   = convert(subprocess.check_output(['pkg-config', '--cflags-only-I', lib]), '-I')
                    , LINKFLAGS = subprocess.check_output(['pkg-config', '--libs-only-other', lib]).split()
                    , LIBPATH   = convert(subprocess.check_output(['pkg-config', '--libs-only-L', lib]), '-L')
                    , LIBS      = convert(subprocess.check_output(['pkg-config', '--libs-only-l', lib]), '-l')
                    )

uxf2png = Builder(action='umlet -action=convert -format=png -filename=$SOURCE',
                  suffix='uxf.png', src_suffix='uxf')

rng2rnc = Builder(action='trang -I rng -O rnc $SOURCE $TARGET',
                  suffix='rnc', src_suffix='rng')
rng2xsd = Builder(action='trang -I rng -O xsd $SOURCE $TARGET',
                  suffix='xsd', src_suffix='rng')
