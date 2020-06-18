#include <stdio.h>
#include <platform_API.h>
#include <compiler_memory_manager.h>
#include <iljit-utils.h>

// My headers
#include <plugin_manager.h>
// End

#define DIM_BUF 2056

static inline void internal_print_plugins (JITINT8 *path, void (*print_plugin)(void *plugin), JITINT8 *plugin_symbol);

void PLUGIN_loadPluginDirectoryNamesToUse (JITINT8 *prefixToUse, JITINT8 **pathToUse, JITINT8 *envName, JITINT8 *dirName) {
    char    *additionalPath;

    /* Check if this is the first time this function is called.
     */
    if ((*pathToUse)[0] != '\0' ) {
        return ;
    }

    /* Fetch the additional paths.
     */
    additionalPath = getenv((char *)envName);

    /* Check if exist the environment variable.
     */
    if (additionalPath != NULL) {
        sprintf((char *)*pathToUse, "%s/lib/iljit/%s/%c%s", prefixToUse, dirName, ENV_VAR_SEPARATOR, additionalPath);
    } else {
        sprintf((char *)*pathToUse, "%s/lib/iljit/%s/", prefixToUse, dirName);
    }

    return ;
}

void PLUGIN_loadPluginsFromDirectory (XanList *pluginsLoaded, JITINT8 *directoryName, JITUINT32 pluginSize, JITINT8 *plugin_symbol) {
    DIR                             *plugin_dir;
    struct dirent                   *info_current_plugin;
    char                            *buf;
    JITINT32 bufLength;
    JITUINT32 directoryNameLength;

    /* Assertions		*/
    assert(pluginsLoaded != NULL);
    assert(directoryName != NULL);
    assert(pluginSize > 0);
    assert(plugin_symbol != NULL);

    /* Open the directory	*/
    plugin_dir = PLATFORM_openDir((char *) directoryName);
    if (plugin_dir == NULL) {
        return ;
    }

    /* Allocate the buffer	*/
    directoryNameLength = STRLEN(directoryName);
    bufLength = 256 + directoryNameLength;
    buf = allocMemory(bufLength);

    /* Load the plugins	*/
    while (1) {
        ildjit_plugin_t         *plugin;
        info_current_plugin = PLATFORM_readDir(plugin_dir);
        if (info_current_plugin == NULL) {
            break;
        }
        if (info_current_plugin->d_name[0] != '.' && str_has_suffix(info_current_plugin->d_name, SHARED_LIB_FILE_EXTENSION)) {

            /* Allocate the plugin		*/
            plugin = allocMemory(pluginSize);
            assert(plugin != NULL);

            /* Write the name of the library*/
            if (directoryName[directoryNameLength - 1] != '/') {
                snprintf(buf, bufLength, "%s/%s", directoryName, info_current_plugin->d_name);
            } else {
                snprintf(buf, bufLength, "%s%s", directoryName, info_current_plugin->d_name);
            }

            /* Open the shared library	*/
            plugin->handle = PLATFORM_dlopen(buf, RTLD_NOW | RTLD_NODELETE);
            if (plugin->handle == NULL) {
                char buf2[DIM_BUF];
                snprintf(buf2, sizeof(buf2), "PLUGIN MANAGER: ERROR= Cannot load plugin %s. Error=%s \n", buf, PLATFORM_dlerror());
                print_err(buf2, 0);
                abort();
            }
            PLATFORM_dlerror();
            plugin->plugin = PLATFORM_dlsym(plugin->handle, (char *) plugin_symbol);
            if (plugin->plugin == NULL) {
                char buf2[DIM_BUF];
                snprintf(buf2, sizeof(buf2), "PLUGIN MANAGER: WARNING= Cannot resolv simbol in plugin %s. Error=%s", buf, PLATFORM_dlerror());
                print_err(buf2, 0);
                abort();
            }

            /* Collect the plugin		*/
            xanList_insert(pluginsLoaded, plugin);
        }
        PLATFORM_dlerror();
    }

    /* Close the directory		*/
    PLATFORM_closeDir(plugin_dir);

    /* Free the memory		*/
    freeMemory(buf);

    /* Return			*/
    return;
}

void PLUGIN_unloadPluginsFromDirectory (XanList *pluginsLoaded, void (*shutdown)(ildjit_plugin_t *plugin, void *data), void *data) {
    XanListItem                     *item;
    ildjit_plugin_t                 *plugin;

    /* Assertions		*/
    assert(pluginsLoaded != NULL);

    /* Unload and free the	*
     * plugins		*/
    item = xanList_first(pluginsLoaded);
    while (item != NULL) {

        /* Fetch the plugin		*/
        plugin = (ildjit_plugin_t *) item->data;
        assert(plugin != NULL);

        /* Call the shutdown function	*/
        if (shutdown != NULL) {
            (*shutdown)(plugin, data);
        }

        /* Close the plugin		*/
        PLATFORM_dlclose(plugin->handle);

        /* Free the plugin		*/
        freeMemory(plugin);

        /* Fetch the next element from	*
         * the list			*/
        item = item->next;
    }

    /* Destroy the list	*/
    xanList_destroyList(pluginsLoaded);

    /* Return		*/
    return;
}

void PLUGIN_loadPluginsFromDirectories (XanList *pluginsLoaded, JITINT8 *directoryPaths, JITUINT32 pluginSize, JITINT8 *plugin_symbol) {
    JITINT8         *buffer;
    JITINT8         *paths;
    JITUINT32 len;

    /* Assertions			*/
    assert(pluginsLoaded != NULL);

    /* Allocate the buffer		*/
    len = STRLEN(directoryPaths);
    buffer = allocFunction(len + 1);
    assert(buffer != NULL);

    /* Iterate over directories	*/
    paths = (JITINT8 *) directoryPaths;
    while (JITTRUE) {

        /* Fetch the current directory	*/
        getFirstPath(paths, buffer);
        if (STRLEN(buffer) == 0) {
            break;
        }

        /* Load the plugins from the    *
         * current directory		*/
        PLUGIN_loadPluginsFromDirectory(pluginsLoaded, buffer, pluginSize, plugin_symbol);

        /* Fetch the next directory	*/
        paths = paths + STRLEN(buffer) + 1;
    }

    /* Free the memory		*/
    freeFunction(buffer);

    /* Return			*/
    return;
}

void PLUGIN_iteratePlugins (JITINT8 *directoryPaths, void (*currentPlugin) (void *plugin), JITINT8 *pluginSymbol) {
    JITINT8         *buffer;
    JITINT8         *paths;
    JITUINT32 	len;

    /* Assertions			*/
    assert(directoryPaths != NULL);

    /* Allocate the buffer		*/
    len = STRLEN(directoryPaths);
    buffer = allocFunction(len + 1);
    assert(buffer != NULL);

    /* Iterate over directories	*/
    paths = (JITINT8 *) directoryPaths;
    while (JITTRUE) {

        /* Fetch the current directory	*/
        getFirstPath(paths, buffer);
        if (STRLEN(buffer) == 0) {
            break;
        }

        /* Load the plugins from the    *
         * current directory		*/
        internal_print_plugins(buffer, currentPlugin, pluginSymbol);

        /* Fetch the next directory	*/
        paths = paths + STRLEN(buffer) + 1;
    }

    /* Free the memory		*/
    freeFunction(buffer);

    /* Return			*/
    return;
}

static inline void internal_print_plugins (JITINT8 *path, void (*print_plugin)(void *plugin), JITINT8 *plugin_symbol) {
    DIR             *plugin_dir;
    struct dirent   *info_current_plugin;

    /* Assertions			*/
    assert(path != NULL);
    assert(print_plugin != NULL);

    plugin_dir = PLATFORM_openDir((const char *) path);
    if (plugin_dir == NULL) {
        PLATFORM_closeDir(plugin_dir);
        return;
    }
    while (1) {
        info_current_plugin = PLATFORM_readDir(plugin_dir);
        if (info_current_plugin == NULL) {
            PLATFORM_closeDir(plugin_dir);
            return;
        }
        if (info_current_plugin->d_name[0] != '.' && str_has_suffix(info_current_plugin->d_name, SHARED_LIB_FILE_EXTENSION)) {
            void            *temp;
            char 		buf[DIM_BUF];

            snprintf(buf, sizeof(buf), "%s%s", path, info_current_plugin->d_name);
            temp = PLATFORM_dlopen(buf, RTLD_NOW);
            if (temp == NULL) {
                return;
            }
            assert(temp != NULL);
            PLATFORM_dlerror();
            void *plugin_handle = PLATFORM_dlsym(temp, (char *)plugin_symbol);
            if (plugin_handle == NULL) {
                PLATFORM_dlclose(temp);
                PLATFORM_closeDir(plugin_dir);
                return;
            }
            assert(plugin_handle != NULL);
            (*print_plugin)(plugin_handle);
        }
        PLATFORM_dlerror();
    }
}
