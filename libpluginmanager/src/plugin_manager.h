#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <jitsystem.h>
#include <xanlib.h>

typedef struct {
    void            *handle;
    void            *plugin;
} ildjit_plugin_t;

void PLUGIN_iteratePlugins (JITINT8 *directoryPaths, void (*currentPlugin) (void *plugin), JITINT8 *pluginSymbol);
void PLUGIN_loadPluginsFromDirectories (XanList *pluginsLoaded, JITINT8 *directoryPaths, JITUINT32 pluginSize, JITINT8 *plugin_symbol);
void PLUGIN_loadPluginsFromDirectory (XanList *pluginsLoaded, JITINT8 *directoryName, JITUINT32 pluginSize, JITINT8 *plugin_symbol);
void PLUGIN_loadPluginDirectoryNamesToUse (JITINT8 *prefixToUse, JITINT8 **pathToUse, JITINT8 *envName, JITINT8 *dirName);
void PLUGIN_unloadPluginsFromDirectory(XanList *pluginsLoaded, void (*shutdown)(ildjit_plugin_t *plugin, void *data), void *data);

#endif
