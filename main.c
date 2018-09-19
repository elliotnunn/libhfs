#include "block.c"
#include "btree.c"
#include "data.c"
#include "file.c"
#include "hfs.c"
#include "low.c"
#include "medium.c"
#include "memcmp.c"
#include "node.c"
#include "os.c"
#include "record.c"
#include "version.c"
#include "volume.c"

#include <Python.h>

static char module_docstring[] =
    "This is the docstring for the module.";
static char elmo_docstring[] =
    "This is the docstring for the elmo function.";

static PyObject *libhfs_elmo(PyObject *self, PyObject *args);

static PyMethodDef module_methods[] = {
    {"elmo", libhfs_elmo, METH_VARARGS, elmo_docstring},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_libhfs(void)
{
    PyObject *m = Py_InitModule3("libhfs", module_methods, module_docstring);
    if (m == NULL)
        return;
}

static PyObject *libhfs_elmo(PyObject *self, PyObject *args)
{
	printf("Hello from the elmo function!\n");
    return Py_None;
}
