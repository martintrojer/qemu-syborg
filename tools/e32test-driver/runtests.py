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

# Provides 'ui' for running RTests on target and generating a report of the results.
# Uses qemuruntest to run the tests and rtestreport to generate the results.

import glob
import sys
import os.path

from optparse import OptionParser

import qemuruntest
import rtestreport

def ParseOptions():
    optParser = OptionParser()
    optParser.add_option("-b", "--board", action="store", type="string", default="syborg")
    optParser.add_option("-c", "--cpu", action="store", type="string", default="cortex-a8")
    optParser.add_option("-d", "--display", action="store_true", dest="displayp")
    optParser.add_option("-i", "--input", action="store", type="string")
    optParser.add_option("-o", "--output", action="store", type="string")
    optParser.add_option("-q", "--qemu", action="store", type="string", dest="qemupath", default="qemu-system-arm.exe")
    optParser.add_option("-r", "--rom", action="store", type="string", default="syborg.e32test.a8.urel.elf")
    optParser.add_option("-s", "--summary", action="store", type="string")

    return optParser.parse_args()

def main():
    errors = False
    (options, args) = ParseOptions()

    if  not options.input:
        gl = glob.glob(options.qemupath)
        if len(gl) == 0:
            gl = glob.glob(options.qemupath + ".exe")

        if len(gl) == 0:
            print >> sys.stderr, "ERROR: can't find qemu executable %s" % (options.qemupath)
            errors = True
        else:
            qemupath = gl[0]

        gl = glob.glob(options.rom)
        if len(gl) == 0:
            print >> sys.stderr, "ERROR: can't find ROM image %s" % (options.rom)
            errors = True
        else:
            rompath = gl[0]

        if errors:
            sys.exit(1)

        output = options.output
        if output == None:
            output = rompath

        runner = qemuruntest.QemuTestRunner(qemupath, options.cpu, rompath, board = options.board, displayp = options.displayp, dataFile = output)
        runner.Run()
    else:
        gl = glob.glob(options.input)
        if len(gl) == 0:
            print >> sys.stderr, "ERROR: can't find input file %s" % (options.input)
            sys.exit(1)

        input = gl[0]
        runner = qemuruntest.PseudoRunner(input)
        
    testReporter = rtestreport.RTestReport(runner, reportFileRoot = options.summary)
    reportFile = False
    status = 1
    try:
        reportFile = testReporter.OpenReportFile()
        status = testReporter.WriteReport(reportFile)
    except Exception, x:
        print >> sys.stderr, x
        status = 1
        if reportFile:
            reportFile.close()

    return status
        
    
if __name__ == "__main__":
    main()
