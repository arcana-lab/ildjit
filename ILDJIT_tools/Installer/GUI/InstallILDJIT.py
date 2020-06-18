#!/usr/bin/python
from PyQt4 import QtCore,QtGui
from ui.MainWindow import MainWindow
#----------------------------------------------------------
if __name__ == '__main__':
    import sys
    app = QtGui.QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
