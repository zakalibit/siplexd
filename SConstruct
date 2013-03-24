env = Environment()
conf = Configure(env)

AddOption('--dbg',
          action='store_true',          
          help='build in debug mode')

AddOption('--libconfig',
           dest='libconfig',
           type='string',
           nargs=1,
           action='store',
           metavar='DIR',
           help='libconfig path')

###########################################################
## Check for dependencies
##########################################################
assert conf.CheckLib('pthread')
assert conf.CheckHeader(['stdlib.h', 'osip2/osip.h'])
assert conf.CheckLib('osip2')
assert conf.CheckLib('config')

###########################################################
## Enable libeXosip2 if installed
###########################################################
if  conf.CheckLibWithHeader('eXosip2', 'eXosip2/eXosip.h', 'c'):
    conf.env.Append(CPPDEFINES=['-DHAS_LIBEXOSIP'])
    conf.env.Append(LIBS=['eXosip2'])

env = conf.Finish()

if env.GetOption('dbg'):
    env.Append(CPPDEFINES=['DEBUG'])
    env.Append(CCFLAGS=['-g'])
else:
    env.Append(CCFLAGS=['-fno-builtin'])

env.Append(CPPDEFINES=['_GNU_SOURCE'])
env.Append(CCFLAGS = ['-std=c99'])
env.Append(CPPPATH=['inc'])
env.Append(LIBS=['pthread', 'osip2', 'config'])

env.Program('siplexd', Glob('src/*.c'))

Decider('MD5-timestamp')
