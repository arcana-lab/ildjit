
MAINTAINERCLEANFILES = Makefile.in config.h.in

libiljitmm_LTLIBRARIES = libiljitmm.la
libiljitmmdir = $(libdir)/iljit

AM_CFLAGS= -I../include $(ILJITMM_ILJITU_CFLAGS) $(ILJITMM_ILJITSM_CFLAGS) $(XAN_CFLAGS) $(COMPILERMEMORYMANAGER_CFLAGS) $(PLATFORM_CFLAGS) -D_GNU_SOURCE

if PROFILE
AM_CFLAGS 	+= -DNDEBUG -DPROFILE -O0 -g
else
if PRINTDEBUG
AM_CFLAGS+= -g -DDEBUG -DPRINTDEBUG -O0
else 
if DEBUG
AM_CFLAGS += -g -DDEBUG -O0
else 
AM_CFLAGS+= -O3 -DNDEBUG
endif
endif
endif

libiljitmm_la_SOURCES=			 							\
		basic_concept_manager.c		basic_concept_manager.h \
		generic_container.c			generic_container.h \
		class_descriptor.c			class_descriptor.h \
		field_descriptor.c			field_descriptor.h \
		method_descriptor.c			method_descriptor.h \
		type_descriptor.c			type_descriptor.h \
		automatic_type_descriptor.c		automatic_type_descriptor.h \
		property_descriptor.c		property_descriptor.h \
		event_descriptor.c			event_descriptor.h \
		var_descriptor.c			var_descriptor.h \
		decoding_tools.c			decoding_tools.h \
		generic_type_rules.c		generic_type_rules.h \
		type_spec_decoding_tool.c	type_spec_decoding_tool.h \
		type_spec_descriptor.c		type_spec_descriptor.h \
		metadata_types_manager.c	metadata_types_manager.h \
		metadata_manager.c			metadata_manager.h			\
		metadata_streams_manager.c		metadata_streams_manager.h		\
		metadata_string_stream_manager.c	metadata_string_stream_manager.h	\
		metadata_us_stream_manager.c		metadata_us_stream_manager.h		\
		metadata_not_stream_manager.c		metadata_not_stream_manager.h		\
		metadata_blob_stream_manager.c		metadata_blob_stream_manager.h		\
		metadata_guid_stream_manager.c		metadata_guid_stream_manager.h		\
		module_table_metadata.c			module_table_metadata.h			\
		module_ref_table_metadata.c		module_ref_table_metadata.h		\
		type_ref_table_metadata.c		type_ref_table_metadata.h		\
		type_def_table_metadata.c		type_def_table_metadata.h		\
		field_table_metadata.c			field_table_metadata.h			\
		method_def_table_metadata.c		method_def_table_metadata.h		\
		param_table_metadata.c			param_table_metadata.h			\
		member_ref_table_metadata.c		member_ref_table_metadata.h		\
		assembly_table_metadata.c		assembly_table_metadata.h		\
		assembly_ref_table_metadata.c		assembly_ref_table_metadata.h		\
		standalone_sig_table_metadata.c		standalone_sig_table_metadata.h		\
		interface_impl_table_metadata.c		interface_impl_table_metadata.h		\
		constant_table_metadata.c		constant_table_metadata.h		\
		custom_attribute_table_metadata.c	custom_attribute_table_metadata.h	\
		property_table_metadata.c		property_table_metadata.h		\
		field_marshal_table_metadata.c		field_marshal_table_metadata.h		\
		decl_security_table_metadata.c		decl_security_table_metadata.h		\
		class_layout_table_metadata.c		class_layout_table_metadata.h		\
		field_layout_table_metadata.c		field_layout_table_metadata.h		\
		event_map_table_metadata.c		event_map_table_metadata.h		\
		event_table_metadata.c			event_table_metadata.h			\
		property_map_table_metadata.c		property_map_table_metadata.h		\
		method_semantics_table_metadata.c	method_semantics_table_metadata.h	\
		method_impl_table_metadata.c		method_impl_table_metadata.h		\
		type_spec_table_metadata.c		type_spec_table_metadata.h		\
		impl_map_table_metadata.c		impl_map_table_metadata.h		\
		field_rva_table_metadata.c		field_rva_table_metadata.h		\
		manifest_resource_table_metadata.c	manifest_resource_table_metadata.h	\
		file_table_metadata.c			file_table_metadata.h			\
		exported_type_table_metadata.c		exported_type_table_metadata.h		\
		nested_class_table_metadata.c		nested_class_table_metadata.h		\
		method_spec_table_metadata.c		method_spec_table_metadata.h		\
		generic_param_table_metadata.c		generic_param_table_metadata.h		\
		generic_param_constraint_table_metadata.c	generic_param_constraint_table_metadata.h		\
		mtm_utils.c				mtm_utils.h				\
		metadata_table_manager.c		metadata_table_manager.h		\
		decoder.h									\
		iljitmm-system.h								\
		metadata_types.h								\
		metadata_types_spec.h

libiljitmm_la_LIBADD= $(ILJITMM_ILJITU_LIBS) $(ILJITMM_ILJITSM_LIBS) $(XAN_LIBS) $(COMPILERMEMORYMANAGER_LIBS) $(PLATFORM_LIBS) -L/usr/lib
libiljitmm_la_LDFLAGS= -fPIC
