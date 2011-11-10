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

# A simple report generator for the results of running an RTest testsuite.
#

import sys
import qemuruntest
import rtest

class RTestReport(object):
    def __init__(self, testRunner, reportFileRoot = None):
        self.testRunner = testRunner
        self.reportFileRoot = reportFileRoot

    def SummariseErrors(self, file = sys.stdout ):
        print >> file, "%d errors were detected while running rom %s :" % (len(self.erroredTests), self.parser.GetRomName())
        for e in self.erroredTests:
            print >> file, e.GetName()
        print >> file, "\n"

    def SummariseFailures(self, file = sys.stdout ):
        print >> file, "%d failures were detected while running rom %s :" % (len(self.failedTests), self.parser.GetRomName())
        for e in self.failedTests:
            print >> file, e.GetName()
            for l in e.ErrorContext():
                print >> file, "\t%s" %(l)
        print >> file, "\n"

    def SummarisePassed(self, file = sys.stdout ):
        print >> file, "The following %d tests passed while running rom %s (times are approximate):" % (len(self.passedTests), self.parser.GetRomName())
        for e in self.passedTests:
            print >> file, e.GetName(), " : ", e.GetTimeTaken(), "seconds"
        print >> file, "\n"
        
    def OpenReportFile(self):
        name = self.reportFileRoot
        if not name:
            name = self.testRunner.GetReportFileName()
        return open(name, 'w')

    def GetReportFileName(self):
        return self.reportFileName
    
    def PrepareReport(self):
        self.parser = rtest.RTestParser(self.testRunner)
        try:
            self.parser.Parse()
        except rtest.NoRTestsFound, x:
            print x
            sys.exit(1)
        self.failedTests = filter(rtest.RTest.Failedp, self.parser.GetRTests())
        self.erroredTests = filter(rtest.RTest.Erroredp, self.parser.GetRTests())
        self.passedTests = filter(rtest.RTest.Passedp, self.parser.GetRTests())

    def WriteReport(self, file = sys.stdout):
        self.PrepareReport()
        print >> file, self.testRunner.GetSummary()
        self.SummariseErrors(file)
        self.SummariseFailures(file)
        self.SummarisePassed(file)
        return 0
        
        
