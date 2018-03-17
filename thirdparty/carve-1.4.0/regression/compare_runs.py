import sys
import os
import sre

from sets import Set

output = sre.compile(r"test_(.*).out")

RUN_1_DIR = sys.argv[1]
RUN_2_DIR = sys.argv[2]

run_1_files = os.listdir(RUN_1_DIR)
run_2_files = os.listdir(RUN_2_DIR)

tests_1 = []
tests_2 = []

for _ in run_1_files:
  m = output.match(_)
  if m is None: continue
  tests_1.append(m.group(1))

for _ in run_2_files:
  m = output.match(_)
  if m is None: continue
  tests_2.append(m.group(1))

tests = Set(tests_1) & Set(tests_2)

print len(tests), 'common tests'

def parseLog(log):
  try:
    timings_idx = log.index('Timings: ')
    totals_idx = log.index('Totals: ')
  except ValueError:
    return {}, None
  timings = log[timings_idx+1:totals_idx-1]
  totals = log[totals_idx+1:-2]
  tot = {}
  # mem = timings[1].split()
  # mem = (float(mem[-5].replace(',','')), float(mem[-2].replace(',','')))
  mem = None
  for line in totals:
    line = line.strip()
    ident, t = line.rsplit(' - ', 1)
    ident = ident.strip()
    t = float(t[:-1])
    tot[ident] = t
  return tot, mem

def PCT(b, a, k):
  if k not in a or k not in b: return '********'
  return '%+6.2f%%' % ((float(b[k]) - float(a[k])) * 100.0 / float(a[k]),)

def compareStats(a, b):
  print '  Exec time:  %s' % (PCT(b, a, 'Application')),
  print '  Parse time: %s' % (PCT(b, a, 'Parse')),
  print '  Eval time:  %s' % (PCT(b, a, 'Eval')),

def compareMem(a, b):
  if a is not None and b is not None:
    print '  Mem usage:  %+6.2f%%' % (PCT(b[0], a[0]),),
    print '  Mem delta:  %+6.2f%%' % (PCT(b[1], a[1]),),

def compare(test, a, b):
  print 'test: %-30s' % (test,),
  if a[0] == b[0]:
    print '    OK',
  else:
    print 'DIFFER',

  a_log = a[1].split('\n')
  a_stats = parseLog(a_log)

  b_log = b[1].split('\n')
  b_stats = parseLog(b_log)

  compareStats(a_stats[0], b_stats[0])
  compareMem(a_stats[1], b_stats[1])
  print

for test in tests:
  a_out = os.path.join(RUN_1_DIR, 'test_%s.out' % (test,))
  a_err = os.path.join(RUN_1_DIR, 'test_%s.err' % (test,))

  b_out = os.path.join(RUN_2_DIR, 'test_%s.out' % (test,))
  b_err = os.path.join(RUN_2_DIR, 'test_%s.err' % (test,))

  compare(test, (open(a_out).read(), open(a_err).read()), (open(b_out).read(), open(b_err).read()))
