from PyQt4 import QtCore, QtGui
from logic.Downloader import Downloader
from config.ConfigManager import Node
from logic.DownloaderThread import DownloaderThread

class DownTableWidget(QtGui.QTableWidget):
    '''This widget displays the list of all the packages in download with the corresponding progress bar.'''
#----------------------------------------------------------
    def __init__(self,parent):
        QtGui.QTableWidget.__init__(self,parent)
        self.setMinimumSize(QtCore.QSize(700,300))
        self.setColumnCount(3)
        self.parent=parent
        labels = ['Package','Version','Progress']
        self.setHorizontalHeaderLabels(labels)
        self.horizontalHeader().setResizeMode(0, QtGui.QHeaderView.Interactive)
        self.setColumnWidth(0,200)
        self.setColumnWidth(2,400)
        self.progressBars = []
        self.downloaders = []
	self.dt = DownloaderThread(self.downloaders)
        self.index=0;
#----------------------------------------------------------
    def myAddRow(self,name,packName,version,type,url):
        '''Add a row the the list and call the downloader.
        @param name: name of the package to be downloaded
        @param packName: package name of the package to be downloaded
        @param version: version of the package
        @param type: extension of the package
        @param url: base url of the main page. The actual url will be: url+packName-version.type'''
        r = self.rowCount()
        self.insertRow(r)
        name_item = QtGui.QTableWidgetItem(name,QtGui.QTableWidgetItem.UserType)
        version_item = QtGui.QTableWidgetItem(version,QtGui.QTableWidgetItem.UserType)
        self.setItem(r,0,name_item)
        self.setItem(r,1,version_item)
        self.progress = QtGui.QProgressBar(self)
        self.progress.setValue(0)
        self.progressBars.append(self.progress)
        self.downloaders.append(Downloader(url+packName+'-'+version+'.'+type+"?download",packName+'-'+version+'.'+type,r,self.directory))
        QtCore.QObject.connect(self.downloaders[r], QtCore.SIGNAL("setValue"), self.setValue, QtCore.Qt.QueuedConnection)
        QtCore.QObject.connect(self.downloaders[r], QtCore.SIGNAL("downloadingError"), self.manageDownError, QtCore.Qt.QueuedConnection)
        QtCore.QObject.connect(self.downloaders[r], QtCore.SIGNAL("downloadCompleted"), self.manageCompletion, QtCore.Qt.QueuedConnection)
        self.setCellWidget(r,2,self.progress)
#----------------------------------------------------------
    def setValue(self,index,percent):
        '''Set the progress of the given progress bar.
        @param index: the identification number of the progress bar to be updated
        @param percent: the value to be set'''
        self.progressBars[index].setValue(percent)
#----------------------------------------------------------
    def manageDownError(self,file,url):
        '''Show the user an error message.
        @param file: the name of the file that has generated the error.
        @param url: the unreachable url'''
        QtGui.QMessageBox.critical(self,self.tr("Download Error!"), self.tr("Download of file "+file+" from "+url+" failed."));
#----------------------------------------------------------
    def manageParsingError(self,file):
        '''Show the user an error message.
        @param file: the name of the file that cannot be parsed.'''
        QtGui.QMessageBox.critical(self,self.tr("Parsing Error!"), self.tr("Parsing of file "+file+" failed."));
#--------------------------------------------------------------
    def manageCompletion(self):
        '''Manage the completion of a subtree.
        If the method has been called as many times as the number of packages a success message is shown to the user.'''
        self.index=self.index+1
        if self.index==self.number:
            QtGui.QMessageBox.information(self,self.tr("Download Completed!"),self.tr("<font><b>Download Completed</b></font>"));
            self.parent.nextButton.setEnabled(True)
#--------------------------------------------------------------
    def startDownload(self,sourceDir):
        '''Start the download of the packages.
        @param sourceDir: the destination folder.'''
        tree=self.parent.parent.tree
        self.directory=sourceDir
        toDownload = [ n for n in tree.getNodes(tree.root)
                        if(n.action==Node.INSTALL)]
        self.number=len(toDownload)
        if self.number==0:
            self.parent.nextButton.setEnabled(True)
        else:
            for node in toDownload:
                self.myAddRow(node.name,node.packName,node.version,node.type,tree.getUrl(node.name))
                self.resizeColumnsToContents()
                self.setColumnWidth(0,200)
                self.setColumnWidth(2,400)
	    self.dt.start()
