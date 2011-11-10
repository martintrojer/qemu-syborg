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

# Implements a 'simple' watchdog built on top of the Timer class from
# the Threading module.
# After a specified interval a WatchDog will invoke a specified function.
# At any point the WatchDog can be Reset, which means the countdown will start from scratch.
# WatchDogs can also be Started and Stopped.
# NB the WatchDog has no idea what the function it invokes will do.

import threading

class WatchDog(object):
    def __init__(self, interval, function):
        self.interval = interval
        self.function = function
        self.Watcher()

    def Watcher(self):
        self.timerThread = threading.Timer(self.interval, self.function)

    def Start(self):
        self.timerThread.start()

    def Stop(self):
        self.timerThread.cancel()
        
    def Reset(self):
        self.Stop()
        self.Watcher()
        self.Start()

