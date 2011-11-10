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

# RTestParser represents the results of running an RTest testsuite on the target in terms of RTest
# objects. Each RTest object captures the LineTimeInfo associated with the test and can determine
# whether the test errored, failed or passed. It also provides further information such as the name of
# the test and how long it took to gather the output from the test (which is a crude estimate of how long
# it took to run the test. It can provide various useful bits of information like the line number in the
# raw test data of a given line and the 'context' of a failure (in terms of the lines in the raw data).

import sys

import re

import qemuruntest

startRTestPattern = re.compile('RTEST TITLE:')
bmSuiteFailurePattern = re.compile('Error:')
endRTestPattern = re.compile('RUNTESTS: Test')
failedRTestPattern = re.compile('RUNTESTS: Test .* FAIL')
erroredRTestPattern = re.compile('RUNTESTS: Test .* ERROR')
nameRTestPattern = re.compile('RUNTESTS: Test (\w+)')
testSuiteEndingPattern = re.compile('RUNTESTS: Elapsed')

class NoRTestsFound(Exception):
    def __init__(self, parser):
        self.parser = parser

    def __str__(self):
        return "No test results found in output from running '%s' rom image" % (self.parser.GetRomName())
    
    def GetParser(self):
        return self.parser

class RTestEndMissing(Exception):
    def __init__(self, rtest):
        self.rtest = rtest

    def __str__(self):
        return "Could not find end of test started on line %d: '%s'" % (self.rtest.GetLineNumber(0), self.rtest.GetLine(0))
    
    def GetParser(self):
        return self.parser

    def GetIndex(self):
        return self.index

class RTestNameMissing(Exception):
    def __init__(self, rtest, line):
        self.rtest = rtest
        self.line = line

    def GetRTest(self):
        return self.rtest
    
    def __str__(self):
        return "Could not find RTest name in line '%s'" % (self.line)
    
    def GetParser(self):
        return self.parser

    def GetLine(self):
        return self.line

class RTest(object):
    def __init__(self, parser, lineTimeInfo, startLine = 0):
        """return new RTest instance"""
        self.parser = parser
        self.lineTimeInfoList = []
        self.lineTimeInfoList.append(lineTimeInfo)
        self.timeTaken = None
        self.result = None
        self.name = None
        self.startLine = startLine
        #line = lineTimeInfo.GetLine()
        #print >> sys.stdout, line

    def __str__(self):
            return self.GetName() + " " + self.GetResult()
        
    def Consume(self,lineTimeInfoList, start):
        newStart = start;
        for i in range(start, len(lineTimeInfoList) - 1):
            lineTimeInfo = lineTimeInfoList[i]
            self.lineTimeInfoList.append(lineTimeInfo)
            line = lineTimeInfo.GetLine()
            newStart = newStart + 1
            if self.EndOfTestp(line):
                break
        else:
            raise RTestEndMissing(self)
        
        return newStart
            
    def EndOfTestp(self,line):
        return re.match(endRTestPattern, line) != None

    def FailedTestp(self,line):
        return re.match(failedRTestPattern, line) != None

    def ErroredTestp(self,line):
        return re.match(erroredRTestPattern, line) != None

    def GetTimeTaken(self):
        if self.timeTaken == None:
            self.timeTaken = self.lineTimeInfoList[-1].GetTime() - self.lineTimeInfoList[0].GetTime()
        return self.timeTaken

    def GetResult(self):
        if self.result == None:
            line = self.lineTimeInfoList[-1].GetLine()
            if self.FailedTestp(line):
                self.result = 'Failed'
            elif self.ErroredTestp(line):
                self.result = 'Errored'
            else:
                self.result = 'Passed'
        return self.result

    def Failedp(self):
        return self.GetResult() == 'Failed'

    def Erroredp(self):
        return self.GetResult() == 'Errored'
    
    def Passedp(self):
        return self.GetResult() == 'Passed'
    
    def GetName(self):
        if self.name == None:
            try:
                self.name = self.ParseName()
            except RTestNameMissing, x:
                print >> sys.stderr, "WARNING: ", x
                self.name = "RTEST @ line %d" % (x.GetRTest().GetLineNumber(0))
        return self.name

    def ParseName(self):
        line = self.lineTimeInfoList[-1].GetLine()
        m = re.match(nameRTestPattern, line)
        if m != None:
            return m.group(1)
        else:
            raise RTestNameMissing(self, line)

    def GetLineNumber(self, i):
        return self.startLine + i

    def GetLine(self, index):
        return self.lineTimeInfoList[index].GetLine()

    def ErrorContext(self):
        if self.Failedp():
            return map(qemuruntest.LineTimeInfo.GetLine, self.lineTimeInfoList[-5:-1])
        else:
            return []
        
    
class RTestParser(object):
    def __init__(self, testRunner, lineTimeInfoList = None):
        self.testRunner = testRunner
        if lineTimeInfoList == None:
            self.lineTimeInfoList = testRunner.GetResults()
        else:
            self.lineTimeInfoList = lineTimeInfoList
        self.rtestList = []
        self.result = None
        
    def Parse(self):
        index = 0
        end = len(self.lineTimeInfoList)
        self.rtestList = []
        testSuiteComplete = False
        testErroredp = False

        # find first test
        while index < end:
            lineTimeInfo = self.lineTimeInfoList[index]
            index += 1;
            line = lineTimeInfo.GetLine()
            if self.StartOfTestp(line):
                break
            if self.ErroredTestp(line):
                self.rtestList.append(RTest(self, lineTimeInfo, index-1))
        else:
            raise NoRTestsFound(self)

        try:
            while index < end:
                # NB making startLine index means that line number are based at 1 rather than 0
                rtest = RTest(self, lineTimeInfo, startLine = index)
                self.rtestList.append(rtest)
                index = rtest.Consume(self.lineTimeInfoList, index)

                if self.TestSuiteEnding(self.lineTimeInfoList[index].GetLine()):
                    testSuiteComplete = True
                    break

                while index < end:
                    lineTimeInfo = self.lineTimeInfoList[index]
                    index += 1;
                    line = lineTimeInfo.GetLine()
                    if self.StartOfTestp(line):
                        break
                    if self.ErroredTestp(line):
                        self.rtestList.append(RTest(self, lineTimeInfo, startLine = index))
        except RTestEndMissing, x:
            print >> sys.stderr, "WARNING: ", x
        return testSuiteComplete
    
    def StartOfTestp(self,line):
        if re.match(startRTestPattern, line) != None:
            return True
        if re.match(bmSuiteFailurePattern, line) != None:
            return True
        return False

    def ErroredTestp(self,line):
        if re.match(erroredRTestPattern, line) != None:
            return True
        
    def TestSuiteEnding(self,line):
        return re.match(testSuiteEndingPattern, line) != None

    def GetRTests(self):
        return self.rtestList

    def GetLine(self,index):
        return self.lineTimeInfoList[index]
    
    def GetRomName(self):
        return self.testRunner.GetRomName()
