#ifndef CLI_MANAGER_H
#define CLI_MANAGER_H

#include <jitsystem.h>
#include <metadata/metadata_types.h>
#include <ir_virtual_machine.h>
#include <jit_metadata.h>

// My headers
#include <lib_delegates.h>
#include <lib_string.h>
#include <lib_stringBuilder.h>
#include <lib_array.h>
#include <lib_reflect.h>
#include <lib_runtimehandle.h>
#include <lib_typedReference.h>
#include <lib_vararg.h>
#include <lib_decimal.h>
#include <lib_thread.h>
#include <lib_culture.h>
#include <lib_net.h>
#include <lib_pinvoke.h>
#include <layout_manager.h>
#include <ilmethod.h>
// End

/* DEFINITIONS OF EXCEPTION TYPES */
#define EXCEPTION_TYPE  1
#define FILTER_TYPE     2
#define FINALLY_TYPE    3
#define FAULT_TYPE      4

/* LABEL_TYPE DEFINITIONS */
#define DEFAULT_LABEL_TYPE              0       // LABEL NOT USED ALSO FOR EXCEPTION-HANDLING
#define EXCEPTION_HANDLING_LABEL_TYPE   1       // LABEL USED FOR EXCEPTION-HANDLING

typedef struct {
    JITBOOLEAN runtimeChecks;                               /**< Flag to turn on or off every checks performed at runtime.  *
	                                                         *  If the flag is set, then the runtime checks will be         *
	                                                         *  performed							*/
    t_cultureManager cultureManager;                        /**< The culture manager				*/
    t_arrayManager arrayManager;
    t_decimalManager decimalManager;                        /**< Manager of the Decimal class                               */
    t_reflectManager reflectManager;
    t_stringManager stringManager;
    t_stringBuilderManager stringBuilderManager;            /**< StringBuilder manager                              */
    t_typedReferenceManager typedReferenceManager;          /**< TypedReference manager				*/
    t_varargManager varargManager;                          /**< Vararg manager					*/
    t_netManager netManager;
    t_delegatesManager delegatesManager;                    /**< Manager of CIL delegates				*/
    t_threadsManager threadsManager;                        /**< Threads manager                                    */
    RuntimeHandleManager runtimeHandleManager;              /**< Manager of all System.Runtime*Handle stuffs	*/
    PinvokeManager	pinvokeManager;				/**< Manager of all pinvoke related stuffs		*/
} CLR_t;

/**
 *
 * All the methods found during the runtime
 */
typedef struct t_methods {
    XanHashTable            *anonymousContainer;
    XanHashTable            *methods;
    XanList                 *container;
    XanHashTable *functionsEntryPoint;                                   /**< The hashtable associating entry points to jit functions       */

    XanList * (*findCompatibleMethods)(struct t_methods *methods, ir_signature_t *signature);
    Method (*fetchOrCreateMethod)(struct t_methods *methods, MethodDescriptor *methodID, JITBOOLEAN isExternallyCallable);
    Method (*fetchOrCreateAnonymousMethod)(struct t_methods *methods, JITINT8 *name);
    JITBOOLEAN (*deleteMethod)(struct t_methods *methods, Method method);
} t_methods;

typedef struct {
    JITUINT32 ID;
    JITUINT32 counter;
    JITUINT32 ir_position;
    JITUINT32 cil_position;
    JITUINT32 byte_offset;
    JITBOOLEAN type;                        /**< THE LABEL-TYPE. IT SHALL BE ONE BETWEEN [0..1]
	                                         *  (SEE THE DEFINED VALUES FOR THE LABEL-TYPE) */
    XanList         *starting_blocks;       /**< THE BLOCKS THAT OWNS THIS LABEL. THIS INFORMATION
	                                         *  IS USED DURING THE CIL-IR TRANSLATION TO KNOW WHICH
	                                         *  TRY-BLOCKS STARTS AT THE ADDRESS OF THIS LABEL.*/
    t_try_handler   *starting_handler;      /**< THIS INFORMATION
	                                         *  IS USED DURING THE CIL-IR TRANSLATION TO KNOW WHICH
	                                         *  CATCH/FILTER/FAULT/FINALLY-BLOCK START AT THIS LABEL.*/
    t_try_handler   *ending_handler;        /**< THE HANDLER THAT ENDS AT THE ADDRESS SPECIFIED BY
	                                         *   THIS LABEL */
    t_try_block     *ending_block;          /**< THE BLOCK THAT ENDS AT THE ADDRESS SPECIFIED BY THIS
	                                         *   LABEL */
    t_stack         *stack;                 /**< THE STACK IMAGE TO RESTORE
	                                         *   (IN THE CASE OF TYPE == DEFAULT_LABEL_TYPE) */
} t_label;

typedef struct CLIManager_t {

    /* Fields                               */
    XanList                         *binaries;              /**< List of binaries decoded				*/
    t_methods methods;                                      /**< All the methods known by the runtime		*/
    struct {
        t_binary_information	*binary;
        Method			method;
    }				entryPoint;		/**< Entry point of the program				*/
    IRVM_t                          *IRVM;			/**< IR virtual machine					*/
    CLR_t				CLR;			/**< Common Language Runtime				*/
    metadataManager_t		*metadataManager;	/**< Manager of CLI metadata				*/
    t_running_garbage_collector	*gc;			/**< Garbage collector					*/
    ILLayout_manager		layoutManager;		/**< Manage the layout of the CIL objects		*/
    struct {
        void (*translateActualMethodSignatureToIR)(struct CLIManager_t *self, MethodDescriptor *method, FunctionPointerDescriptor *signature, ir_signature_t *ir_method);
        Method (*newMethod)(struct CLIManager_t *self, MethodDescriptor *methodID, JITBOOLEAN isExternallyCallable);
        Method (*newAnonymousMethod)(struct CLIManager_t *self, JITINT8 *name);
        ir_symbol_t * (*getTypeDescriptorSymbol)(struct CLIManager_t *self, TypeDescriptor *type);
        ir_symbol_t * (*getFieldDescriptorSymbol)(struct CLIManager_t *self, FieldDescriptor *field);
        ir_symbol_t * (*getMethodDescriptorSymbol)(struct CLIManager_t *self, MethodDescriptor *method);
        ir_symbol_t * (*getIndexOfTypeDescriptorSymbol)(struct CLIManager_t *self, TypeDescriptor *type);
        ir_symbol_t * (*getIndexOfMethodDescriptorSymbol)(struct CLIManager_t *self, MethodDescriptor *method);
    } translator;

    /* Methods provided by the iljit program	*/
    Method			(*fetchOrCreateMethod)		(MethodDescriptor *method);
    void			(*throwExceptionByName)		(JITINT8 *nameSpace, JITINT8 *typeName);
    void			(*ensureCorrectJITFunction)	(Method method);
} CLIManager_t;

void init_CLIManager (CLIManager_t *lib, IRVM_t *irVM, t_running_garbage_collector *gc, Method (*fetchOrCreateMethod)(MethodDescriptor *method), void (*throwExceptionByName)(JITINT8 *nameSpace, JITINT8 *typeName), void (*ensureCorrectJITFunction)(Method method), JITBOOLEAN runtimeChecks, IR_ITEM_VALUE buildDelegateFunctionPointer, JITINT8 *machineCodePath);
void CLIMANAGER_initializeCLR (void);

t_binary_information* CLIMANAGER_fetchBinaryByNameAndPrefix(CLIManager_t *cliManager, JITINT8 *assemblyName, JITINT8 *assemblyPrefix);
JITBOOLEAN CLIMANAGER_binaryIsDecoded(CLIManager_t *cliManager, t_binary_information *binary_info);
void CLIMANAGER_insertDecodedBinary (t_binary_information *binary_info);
void CLIMANAGER_setEntryPoint (CLIManager_t *cliManager, t_binary_information *entry_point);

CLRType * CLIMANAGER_getCLRType (TypeDescriptor *type);

void CLIMANAGER_shutdown (CLIManager_t *lib);

void libclimanagerCompilationFlags (char *buffer, int bufferLength);
void libclimanagerCompilationTime (char *buffer, int bufferLength);
char * libclimanagerVersion ();

#endif
