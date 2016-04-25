/* yaosp bindings for python
 *
 * Copyright (c) 2009 Attila Magyar
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <Python.h>
#include <yaosp/sysinfo.h>
#include <yaosp/time.h>
#include <yaosp/debug.h>
#include <yaosp/ipc.h>

#define 	MODULE_NAME 	"_yaosp"
#define 	EXCEPTION_NAME	"_yaosp.error"

static PyObject *yaospError; // Exception object

static PyObject *yaosp_get_memory_info( PyObject *self, PyObject *args ) {
	int error;
	memory_info_t memory_info;

	error = get_memory_info( &memory_info );

	if ( error < 0 ) {
		PyErr_SetString(yaospError, "Can't get memory information");
		return NULL;
	}

	uint32_t total_mem = memory_info.total_page_count * getpagesize();
	uint32_t free_mem = memory_info.free_page_count * getpagesize();

	return Py_BuildValue("(ii)", total_mem, free_mem) ;
}

static PyObject *yaosp_get_processor_info( PyObject *self, PyObject *args ) {
	uint32_t i;
	uint32_t processor_count;

	processor_info_t* info;
	processor_info_t* info_table;

	PyObject *pylist, *item;

	processor_count = get_processor_count();

	if ( processor_count == 0 ) {
		Py_RETURN_NONE;
	}

	info_table = ( processor_info_t* )malloc( sizeof( processor_info_t ) * processor_count );

	if ( info_table == NULL ) {
		return PyErr_NoMemory();
	}

	processor_count = get_processor_info( info_table, processor_count );

	pylist = PyList_New(0);

	if ( pylist == NULL ) {
		goto cp_err;
	}

	for ( i = 0, info = info_table; i < processor_count; i++, info++ ) {
		if ( ( !info->present ) || ( !info->running ) ) {
			continue;
		}

		item = Py_BuildValue("{s:s,s:i}",
			"model", info->name,
			"speed", ( uint32_t )( info->core_speed / 1000000 ) );

		PyList_Append( pylist, item );
	}

cp_err:
	free( info_table );

	return pylist;
}

static PyObject *yaosp_get_kernel_info( PyObject *self, PyObject *args ) {
	int error;
	char ver[ 32 ];
	char build[ 128 ];
	kernel_info_t kernel_info;

	error = get_kernel_info( &kernel_info );

	if ( error < 0 ) {
		PyErr_SetString(yaospError, "Can't get kernel information");
		return NULL;
	}

	snprintf( ver, sizeof(ver), "%d.%d.%d",
		kernel_info.major_version,
		kernel_info.minor_version,
		kernel_info.release_version
	);
	snprintf( build, sizeof(build), "%s %s",
		kernel_info.build_date,
		kernel_info.build_time
	);

	return Py_BuildValue("{s:s,s:s}", "version", ver, "build", build  );
}

static PyObject *yaosp_get_module_info( PyObject *self, PyObject *args ) {
	int error;
	uint32_t i;
	uint32_t module_count;
	module_info_t* info;
	module_info_t* info_table;
	PyObject *pylist, *item;

	module_count = get_module_count();

	if ( module_count > 0 ) {
		info_table = ( module_info_t* )malloc( sizeof( module_info_t ) * module_count );

		if ( info_table == NULL ) {
			return PyErr_NoMemory();
		}

		error = get_module_info( info_table, module_count );

		if ( error < 0 ) {
			free( info_table );
			PyErr_SetString(yaospError, "Can't get module information");
			return NULL;
		}

		pylist = PyList_New( module_count );

		if ( pylist == NULL ) {
			free( info_table );
			return NULL;
		}

		for ( i = 0, info = info_table; i < module_count; i++, info++ ) {
			item = Py_BuildValue( "s", info->name );
			PyList_SetItem( pylist, i, item );
		}

		free( info_table );

		return pylist;
    }

    return Py_BuildValue( "()" );
}

static PyObject *yaosp_get_process_info( PyObject *self, PyObject *args ) {
	uint32_t process_count;
	process_info_t* process;
	process_info_t* process_table;
	PyObject *pylist, *item;

	process_count = get_process_count();

	if ( process_count > 0 ) {
		uint32_t i;

		process_table = ( process_info_t* )malloc( sizeof( process_info_t ) * process_count );
		if ( process_table == NULL ) {
			return PyErr_NoMemory();
		}

		process_count = get_process_info( process_table, process_count );

		pylist = PyList_New( process_count );

		if (pylist == NULL ) {
			free( process_table );
			return NULL;
		}

		for ( i = 0, process = process_table; i < process_count; i++, process++ ) {
			item = Py_BuildValue("{s:i,s:s,s:K,s:K}", // TODO warning 'K' is an undocumented format string value
				"id", process->id,
				"name", process->name,
				"vmem",	process->vmem_size,
				"pmem", process->pmem_size
			);

			PyList_SetItem( pylist, i, item );
		}

		free( process_table );

		return pylist;
	}

	return Py_BuildValue("()");
}

static PyObject *yaosp_get_thread_info( PyObject *self, PyObject *args ) {
	uint32_t thread_count;
	thread_info_t* thread;
	thread_info_t* thread_table;
	int pid;
	PyObject *pylist, *item;

	if (!PyArg_ParseTuple( args, "i", &pid ) ) {
		return NULL;
	}

	thread_count = get_thread_count_for_process( pid );

	if ( thread_count > 0 ) {
		uint32_t i;

		thread_table = ( thread_info_t* )malloc( sizeof( thread_info_t ) * thread_count );

		if ( thread_table == NULL ) {
			return PyErr_NoMemory();
		}

		thread_count = get_thread_info_for_process( pid, thread_table, thread_count );

		pylist = PyList_New(thread_count);

		if ( pylist == NULL ) {
			free( thread_table );
			return NULL;
		}

		for ( i = 0, thread = thread_table; i < thread_count; i++, thread++ ) {
			item = Py_BuildValue("{s:i,s:s,s:i,s:K}", // TODO warning 'K' is an undocumented format string value
				"id", thread->id,
				"name",	thread->name,
				"state", thread->state,
				"cputime", thread->cpu_time
			);

			PyList_SetItem(pylist, i, item);
		}

		free( thread_table );

		return pylist;
	}

	return Py_BuildValue("()");
}

static PyObject *yaosp_dbprint( PyObject *self, PyObject *args ) {
	const char* txt;

	if ( !PyArg_ParseTuple(args, "s", &txt) ) {
		return NULL;
	}

	dbprintf( "%s\n", txt );

	Py_RETURN_NONE;
}

static PyObject *yaosp_system_time( PyObject *self, PyObject *args ) {
	time_t sys_time = get_system_time();

	return Py_BuildValue("K", sys_time) ;
}

static PyObject *yaosp_boot_time( PyObject *self, PyObject *args ) {
	time_t boot_time = get_boot_time();

	return Py_BuildValue("K", boot_time) ;
}

static PyObject *yaosp_uptime( PyObject *self, PyObject *args ) {
	time_t sys_time = get_system_time();
	time_t boot_time = get_boot_time();

	return Py_BuildValue("K", sys_time - boot_time) ;
}

static PyObject *yaosp_ipc_create_port( PyObject *self, PyObject *args ) {
	ipc_port_id id = create_ipc_port();
	return Py_BuildValue("i", id) ;
}

static PyObject *yaosp_ipc_send_message( PyObject *self, PyObject *args ) {
	ipc_port_id id;
	uint32_t code;
	int result, len;
	const char* data;

	if ( !PyArg_ParseTuple( args, "iis#", &id, &code, &data, &len ) ) {
		return NULL;
	}

	result = send_ipc_message( id, code, (void*)data, len );

	return Py_BuildValue( "i",result ) ;
}

static PyObject *yaosp_ipc_receive_message( PyObject *self, PyObject *args ) {
	ipc_port_id id;
	uint32_t code;
	size_t size;
	int ptimeout;
	uint64_t timeout;
	int result;
	char* buffer;

	if ( !PyArg_ParseTuple( args, "iii", &id, &size, &ptimeout ) ) {
		return NULL;
	}

	if ( ptimeout < 0 ) {
		timeout = INFINITE_TIMEOUT;
	}
	else {
		timeout = (uint64_t)ptimeout * 1000;
	}

	buffer = (char*)malloc( size * sizeof(char) +1 ); // allocate +1 byte for chr #0
	if ( buffer == NULL )  {
		return PyErr_NoMemory();
	}

	result = recv_ipc_message( id, &code, buffer, size,  timeout );

	if ( result <= 0 )  {
		code = -1;
		free(buffer);
		return Py_BuildValue( "(i,i,s)", result, code, "" );
	}

	buffer[ size ] = 0; // terminate string with #0

	// return a tupple (len, code, data[:len],)
	PyObject *retVal = Py_BuildValue( "(i,i,s#)", result, code, buffer, result );
	free(buffer);

	return retVal;
}

static PyObject *yaosp_register_named_ipc_port( PyObject *self, PyObject *args ) {
	ipc_port_id id;
	const char *name;
	int result;

	if ( !PyArg_ParseTuple( args, "si", &name, &id ) ) {
		return NULL;
	}

	result = register_named_ipc_port( name, id );
	return Py_BuildValue( "i", result ) ;
}

static PyObject *yaosp_get_named_ipc_port( PyObject *self, PyObject *args ) {
	ipc_port_id id;
	const char *name;
	int result;

	if ( !PyArg_ParseTuple( args, "s", &name ) ) {
		return NULL;
	}

	result = get_named_ipc_port( name, &id );

	if ( result >= 0 ) {
		return Py_BuildValue( "i", id ) ;
	}
	return Py_BuildValue( "i", result ) ;
}

// TODO convert boottime, systemtime, uptime to datetime format

static PyMethodDef yaospMethods[] = {
	{"memory_info", yaosp_get_memory_info, METH_VARARGS, "Get total and free memory"},
	{"processor_info", yaosp_get_processor_info, METH_VARARGS, "Get processor information"},
	{"kernel_info", yaosp_get_kernel_info, METH_VARARGS, "Get kernel information"},
	{"module_info", yaosp_get_module_info, METH_VARARGS, "Get module information"},
	{"process_info", yaosp_get_process_info, METH_VARARGS, "Get process list"},
	{"thread_info", yaosp_get_thread_info, METH_VARARGS, "Get thread info for the specified pid"},
	{"dbprint", yaosp_dbprint, METH_VARARGS, "Debug print"},
	{"boot_time", yaosp_boot_time, METH_VARARGS, "Boot time"},
	{"system_time", yaosp_system_time, METH_VARARGS, "System time"},
	{"uptime", yaosp_uptime, METH_VARARGS, "System uptime"},
	{"create_ipc_port", yaosp_ipc_create_port, METH_VARARGS, "Create IPC port"},
	{"send_ipc_message", yaosp_ipc_send_message, METH_VARARGS, "Send binary data via IPC"},
	{"recv_ipc_message", yaosp_ipc_receive_message, METH_VARARGS, "Receive IPC message"},
	{"register_named_ipc", yaosp_register_named_ipc_port, METH_VARARGS, "Register named IPC port"},
	{"get_named_ipc", yaosp_get_named_ipc_port, METH_VARARGS, "Get named IPC port"},
	{NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC inityaosp(void) {
	PyObject *module = Py_InitModule( MODULE_NAME , yaospMethods );

	if (module == NULL) {
		return;
	}

	// Create and register yaosp exception class
	yaospError = PyErr_NewException( EXCEPTION_NAME , NULL, NULL);
	Py_INCREF( yaospError );

	PyModule_AddObject( module, "error", yaospError );
}
