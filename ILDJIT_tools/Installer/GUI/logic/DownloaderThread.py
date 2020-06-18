from PyQt4 import QtCore
class DownloaderThread(QtCore.QThread):
    '''This class is used to start all the download threads.'''
#--------------------------------------------------------------
    def __init__(self,downloadersList):
        '''@param url: the base url,
        @param file: the file to be downloded,
        @param index: the number of the progress bar,
        @param folder: the directory where to put the downloaded file'''
        QtCore.QThread.__init__(self)
        self.downloaders=downloadersList
#--------------------------------------------------------------
    def run(self):
        '''Perform the download'''
        groupSize=9
        count=0
        for count in range((len(self.downloaders)/groupSize) + 1):
                for down in self.downloaders[count*groupSize:min([count*groupSize + groupSize, len(self.downloaders)])]:
                        down.start()
                for down in self.downloaders[count*groupSize:min([count*groupSize + groupSize, len(self.downloaders)])]:
                        down.wait()
