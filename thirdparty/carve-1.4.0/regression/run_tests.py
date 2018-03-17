import sys
import os
import subprocess
import re
import select
import string

comment = re.compile(r'\s*#.*')
assignment = re.compile(r'^([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)')
continuation = re.compile(r'^\s')

INTERSECT = sys.argv[1]
TESTS = sys.argv[2]
TGT_DIR = sys.argv[3]

if not os.path.exists(TGT_DIR):
  os.makedirs(TGT_DIR)

pre = ''
assignments = {}

SHARK = os.path.exists('/usr/bin/shark') and '/usr/bin/shark' or None
SHARK = None
TIME = None

def runcmd(CMD):
  cmd = subprocess.Popen(CMD, stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
  cout, cerr = cmd.stdout, cmd.stderr
  cmd.stdin.close()

  r_out = []
  r_err = []
  streams = [ cout, cerr ]
  while len(streams):
    ready = select.select(streams, [], [])

    if cout in ready[0]:
      r_out.append(os.read(cout.fileno(), 1024))
      if not len(r_out[-1]): streams.remove(cout)

    if cerr in ready[0]:
      r_err.append(os.read(cerr.fileno(), 1024))
      if not len(r_err[-1]): streams.remove(cerr)

  exitcode = cmd.wait()

  r_out = ''.join(r_out)
  r_err = ''.join(r_err)

  return exitcode, r_out, r_err


WITH_OPROFILE = True

if WITH_OPROFILE:
  search_path = os.environ['PATH'].split(os.pathsep)
  for s in search_path:
    if os.path.isfile(os.path.join(s, 'opcontrol')):
      break
  else:
    print >>sys.stderr, 'could not find opcontrol. disabling oprofile'
    WITH_OPROFILE = False

def opcontrol(*args):
  CMD = ('sudo', 'opcontrol') + tuple(args)
  exitcode, r_out, r_err = runcmd(CMD)

def opreport(path, output):
  CMD = ('opreport', '-l', path)
  exitcode, r_out, r_err = runcmd(CMD)
  if r_out == '' and r_err != '':
    r_out = 'ERROR:\n' + r_err
  open(output, 'w').write(r_out)


def run(cmd):
  test_name, args, op = cmd.split('|')
  test_name = test_name.strip()
  args = args.strip().split()
  op = string.Template(op.strip()).substitute(**assignments)
  print >>sys.stderr, test_name, '...',

  if SHARK:
    CMD = (SHARK, '-o', os.path.join(TGT_DIR, 'prof_%s' % (test_name,)), '-G', '-i', '-1', '-c', '13') + (INTERSECT,) + tuple(args) + (op,)
  elif TIME:
    CMD = (TIME, '-v', '-o', os.path.join(TGT_DIR, 'time_%s' % (test_name,))) + (INTERSECT,) + tuple(args) + (op,)
  else:
    CMD = (INTERSECT,) + tuple(args) + (op,)

  if WITH_OPROFILE:
    opcontrol('--reset')

  exitcode, r_out, r_err = runcmd(CMD)

  if WITH_OPROFILE:
    opreport(os.path.abspath(INTERSECT), os.path.join(TGT_DIR, 'oprofile_%s.out' % (test_name,)))
  
  open (os.path.join(TGT_DIR, 'test_%s.out' % (test_name,)), 'w').write(r_out)
  open (os.path.join(TGT_DIR, 'test_%s.err' % (test_name,)), 'w').write(r_err)
  if exitcode:
    print >>sys.stderr, 'FAIL'
  else:
    print >>sys.stderr, 'PASS'

if WITH_OPROFILE:
  opcontrol('--start')

c = []
def consume():
  if len(c):
    cmd = '\n'.join(c)
    m = assignment.match(cmd)
    if m is not None:
      k, v = m.groups()
      assignments[k] = eval(v)
    else:
      run(re.sub(r'\s+', ' ', cmd))
    c[:] = []

for cmd in [ _ for _ in open(TESTS) if comment.sub('', _).strip() ]:
  cmd = cmd.rstrip('\n')

  if cmd == '':
    consume()
    continue

  if continuation.match(cmd):
    c.append(cmd.lstrip())
    continue

  consume()

  c.append(cmd)

consume()

if WITH_OPROFILE:
  opcontrol('--shutdown')
