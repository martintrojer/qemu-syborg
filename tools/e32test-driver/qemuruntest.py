#
# Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Launch QEMU for SVP with a specified ROM image and core using a COM port for IO
# Uses Python's Popen class for process control.
# Test output is captured by invoking QEMU in -nographic mode. This redirects serial output to stdout which can then be PIPE'd
# using Popen.
# Tests which hang(or crash into the the crash debugger) are detected using a trivial 'watchdog'. IWBN to use e.g. 'select'
# with a timeout, but this won't work on Windoze, which only supports timeout's on sockets. If the watchdog timeouts out QEMU is killed.
# When the test suite runs to (recognizable) completion QEMU is killed. Unfortunately this appears to require an OS specific
# solution. NB unrecognized completion will result in the watchdog killing QEMU.
# QemuTestRunner collects  output into LineTimeInfo objects. These record the time at which each line was received. The time can be used as a crude
# measure of how long a test took to execute.
# The raw data gathered from running the tests can be retrieved using GetResults. This returns a list of LineTimeInfo objects.

import sys
mswindows = (sys.platform == "win32")

# import the following so we can kill QEMU

if mswindows:
#    import win32api
    import signal
else:
    import signal

import os

import time

import re

import subprocess
from subprocess import *

from stat import *

import watchdog

__all__ = ["QemuTestRunner"]

class LineTimeInfo(object):
    def __init__(self, line, atime):
        self.line = line
        self.time = atime

    def GetLine(self):
        return self.line

    def GetTime(self):
        return self.time

class QemuTestRunner(object):
    def __init__(self, qemupath, cpu, rompath, board = 'syborg', endOfTestFn=None, displayp=False, dataFile = None):
        """Create new QemuTestRunner instance."""
        self.qemupath = qemupath
        self.board = board
        self.cpu = cpu
        self.rompath = rompath
        self.endOfTestFn = endOfTestFn
        self.displayp = displayp
        #self.cmd = qemupath + " -M syborg -cpu " + cpu + " -kernel " + rompath + " -nographic"
        self.cmd = "%s -M %s -cpu %s -kernel %s -nographic" % (qemupath, board, cpu, rompath)
        self.endOfTestPattern = re.compile('RUNTESTS: Completed test script')
        self.lineTimeInfo = []
        self.id = None
        self.dataFile = dataFile
        self.watchdog = watchdog.WatchDog(900, lambda : self.KillSim())
        
    def Run(self):
        self.lineTimeInfo = []
        output = False
        self.timeStarted = time.gmtime()
        self.id = time.strftime("%d-%m%--%Y-%H-%M-%S", self.timeStarted)
        if self.dataFile != None:
            filename = self.dataFile + "-" + self.GetRunId() + "-data.txt"
            self.dataFileName = filename
            output = open(filename, 'wb')
        p = Popen( self.cmd, stdin=PIPE, stdout=PIPE, stderr=PIPE )
        self.popen = p
        stdin = p.stdin
        stdout = p.stdout
        stop = False

        if self.displayp:
            print >> sys.stdout, self.cmd
            
        self.watchdog.Start()
        try:
            while p.poll() == None and not stop:
                line = stdout.readline()
                atime = time.clock()
                self.watchdog.Reset()
                if self.displayp and p.returncode == None:
                    print >> sys.stdout , line
                if output and p.returncode == None:
                    print >> output , line
                if p.returncode == None:
                    self.lineTimeInfo.append(LineTimeInfo(line, atime))
                    if self.endOfTestFn != None:
                        stop = self.endOfTestFn(line)
                    else:
                        stop = self.EndOfTestp(line)
        finally:
            self.timeEnded = time.gmtime()
            self.watchdog.Stop()
            if output:
                output.close()
            self.testSuiteFinished = stop;
                
        if p.returncode == None:
            self.KillSim()
            return True
        else:
            return False

    def GetDataFileName(self):
        return self.dataFileName

    def TestSuiteFinishedp(self):
        return self.testSuiteFinished
    
    def EndOfTestp(self, l):
        return re.match(self.endOfTestPattern,l) != None

    def KillSim(self):
#        if mswindows:
#            win32api.TerminateProcess(int(self.popen._handle), -1)
#        else:
            os.kill(slef.popen.pid, signal.SIGKILL)

    def GetResults(self):
        return self.lineTimeInfo

    def GetRomName(self):
        return self.rompath

    def GetRunId(self):
        return self.id

    def GetSummary(self):
        simStat = os.stat(self.qemupath)
        simSummary = "QEMU Executable: %s size: %d creation time: %d\n" % (self.qemupath, simStat[ST_SIZE], simStat[ST_CTIME])
        boardSummary ="Board: %s\n" %  self.board
        cpuSummary = "CPU: %s\n" % self.cpu
        romStat = os.stat(self.rompath)
        romSummary = "ROM image: %s size: %d creation date: %d\n" % (self.rompath, romStat[ST_SIZE], romStat[ST_CTIME])
        timeFormat = "%d-%m%--%Y %H:%M:%S"
        startTime = "Start time: " + time.strftime(timeFormat, self.timeStarted) + "\n"
        endTime = "End time: " + time.strftime(timeFormat, self.timeEnded) + "\n"
        status = "Testsuite did not complete\n"
        if self.TestSuiteFinishedp():
            status = "Testsuite completed\n"
        return simSummary + boardSummary + cpuSummary + romSummary + startTime + endTime + status

    def GetReportFileName(self):
        return self.GetRomName() + "-" + self.GetRunId() + "-results-summary.txt"

class PseudoRunner(QemuTestRunner):
    def __init__(self, input):
        #self.qemupath = qemupath
        #self.board = board
        #self.cpu = cpu
        #self.rompath = rompath
        #self.endOfTestFn = endOfTestFn
        #self.displayp = displayp
        #self.cmd = qemupath + " -M syborg -cpu " + cpu + " -kernel " + rompath + " -nographic"
        #self.endOfTestPattern = re.compile('RUNTESTS: Completed test script')
        self.lineTimeInfo = []
        #self.id = None
        self.dataFile = input
        for line in open(input):
            self.lineTimeInfo.append(LineTimeInfo(line, 0))
        
    def GetRomName(self):
        return "Unknown"

    def GetRunId(self):
        return None

    def GetSummary(self):
        return ""

    def GetReportFileName(self):
        return "Runtest-Summary.txt"
