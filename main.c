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

#define PY_SSIZE_T_CLEAN 1
#include <Python.h>

static const char NAME_HFSVOL[] = "hfsvol";
static const char NAME_HFSDIR[] = "hfsdir";

static PyObject *wrap_mount(PyObject *self, PyObject *args)
{
    char *arg_path; int arg_pnum; int arg_flags;
    if(!PyArg_ParseTuple(args, "sii", &arg_path, &arg_pnum, &arg_flags)) return NULL;
    return PyCapsule_New((void *)hfs_mount(arg_path, arg_pnum, arg_flags), NAME_HFSVOL, NULL);
}

static PyObject *wrap_flush(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_flush(arg_vol)) return NULL;
    return Py_None;
}

static PyObject *wrap_flushall(PyObject *self)
{
    hfs_flushall();
    return Py_None;
}

static PyObject *wrap_umount(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_umount(arg_vol)) return NULL;
    return Py_None;
}

static PyObject *wrap_umountall(PyObject *self)
{
    hfs_umountall();
    return Py_None;
}

static PyObject *wrap_getvol(PyObject *self, PyObject *args)
{
    char *arg_vol;
    if(!PyArg_ParseTuple(args, "y", &arg_vol)) return NULL;
    return PyCapsule_New((void *)hfs_getvol(arg_vol), NAME_HFSVOL, NULL);
}

static PyObject *wrap_setvol(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    hfs_setvol(arg_vol);
    return Py_None;
}

static PyObject *wrap_vstat(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    hfsvolent ret_volent;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_vstat(arg_vol, &ret_volent)) return NULL;
    return Py_BuildValue("y#", (char *)(&ret_volent), sizeof(ret_volent));
}

static PyObject *wrap_vsetattr(PyObject *self, PyObject *args) // problems
{
    hfsvol *arg_vol; PyObject *arg_vol_c; hfsvolent *arg_ent; Py_ssize_t arg_ent_len;
    if(!PyArg_ParseTuple(args, "Oy#", &arg_vol_c, &arg_ent, &arg_ent_len)) return NULL;
    if(arg_ent_len != sizeof(*arg_ent)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_vsetattr(arg_vol, arg_ent)) return NULL;
    return Py_None;
}

static PyObject *wrap_chdir(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_chdir(arg_vol, arg_path)) return NULL;
    return Py_None;
}

static PyObject *wrap_getcwd(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    return Py_BuildValue("l", hfs_getcwd(arg_vol));
}

static PyObject *wrap_setcwd(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; long arg_id;
    if(!PyArg_ParseTuple(args, "Ol", &arg_vol_c, &arg_id)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_setcwd(arg_vol, arg_id)) return NULL;
    return Py_None;
}

static PyObject *wrap_dirinfo(PyObject *self, PyObject *args) // returns name in bytes object!
{
    hfsvol *arg_vol; PyObject *arg_vol_c; long argret_id;
    long ret_id; char ret_name[32];
    if(!PyArg_ParseTuple(args, "Ol", &arg_vol_c, &argret_id)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    if(hfs_dirinfo(arg_vol, &argret_id, ret_name)) return NULL;
    return Py_BuildValue("ly", argret_id, ret_name);
}

static PyObject *wrap_opendir(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    hfsdir *ret_dir;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path)) return NULL;
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL))) return NULL;
    return PyCapsule_New((void *)hfs_opendir(arg_vol, arg_path), NAME_HFSDIR, NULL);
}

static PyObject *wrap_readdir(PyObject *self, PyObject *args)
{
    hfsvol *arg_dir; PyObject *arg_dir_c;
    hfsdirent ret_ent;
    if(!PyArg_ParseTuple(args, "O", &arg_dir_c)) return NULL;
    if(arg_dir_c == Py_None) arg_dir = NULL;
    else if(!(arg_dir = PyCapsule_GetPointer(arg_dir_c, NAME_HFSDIR))) return NULL;
    if(hfs_readdir(arg_dir, &ret_dirent)) return NULL;
    return Py_BuildValue("y#", (char *)(&ret_dirent), sizeof(ret_dirent));
}

static PyObject *wrap_closedir(PyObject *self, PyObject *args)
{
    hfsvol *arg_dir; PyObject *arg_dir_c;
    if(!PyArg_ParseTuple(args, "O", &arg_dir_c)) return NULL;
    if(arg_dir_c == Py_None) arg_dir = NULL;
    else if(!(arg_dir = PyCapsule_GetPointer(arg_dir_c, NAME_HFSDIR))) return NULL;
    if(hfs_closedir(arg_dir)) return NULL;
    return Py_None;
}

static PyMethodDef module_methods[] = {
// Volume routines
    {"mount", wrap_mount, METH_VARARGS, ""},
    {"flush", wrap_flush, METH_VARARGS, ""},
    {"flushall", wrap_flushall, METH_NOARGS, ""},
    {"umount", wrap_umount, METH_VARARGS, ""},
    {"umountall", wrap_umountall, METH_NOARGS, ""},
    {"getvol", wrap_getvol, METH_VARARGS, ""},
    {"setvol", wrap_setvol, METH_VARARGS, ""},
    {"vstat", wrap_vstat, METH_VARARGS, ""},
    {"vsetattr", wrap_vsetattr, METH_VARARGS, ""},
// Directory routines
    // {"chdir", wrap_chdir, METH_VARARGS, ""},
    // {"getcwd", wrap_getcwd, METH_VARARGS, ""},
    // {"setcwd", wrap_setcwd, METH_VARARGS, ""},
    // {"dirinfo", wrap_dirinfo, METH_VARARGS, ""},
    // {"opendir", wrap_opendir, METH_VARARGS, ""},
    // {"readdir", wrap_readdir, METH_VARARGS, ""},
    // {"closedir", wrap_closedir, METH_VARARGS, ""},
// File routines
    // {"create", wrap_create, METH_VARARGS, ""},
    // {"open", wrap_open, METH_VARARGS, ""},
    // {"setfork", wrap_setfork, METH_VARARGS, ""},
    // {"getfork", wrap_getfork, METH_VARARGS, ""},
    // {"read", wrap_read, METH_VARARGS, ""},
    // {"write", wrap_write, METH_VARARGS, ""},
    // {"truncate", wrap_truncate, METH_VARARGS, ""},
    // {"seek", wrap_seek, METH_VARARGS, ""},
    // {"close", wrap_close, METH_VARARGS, ""},
// Catalog routines
    // {"stat", wrap_stat, METH_VARARGS, ""},
    // {"fstat", wrap_fstat, METH_VARARGS, ""},
    // {"setattr", wrap_setattr, METH_VARARGS, ""},
    // {"fsetattr", wrap_fsetattr, METH_VARARGS, ""},
    // {"mkdir", wrap_mkdir, METH_VARARGS, ""},
    // {"rmdir", wrap_rmdir, METH_VARARGS, ""},
    // {"delete", wrap_delete, METH_VARARGS, ""},
    // {"rename", wrap_rename, METH_VARARGS, ""},
// Media routines
    // {"zero", wrap_zero, METH_VARARGS, ""},
    // {"mkpart", wrap_mkpart, METH_VARARGS, ""},
    // {"nparts", wrap_nparts, METH_VARARGS, ""},
    // {"format", wrap_format, METH_VARARGS, ""},

    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef libhfs_module = {
    PyModuleDef_HEAD_INIT,
    "libhfs",
    "Test Module",
    -1,
    module_methods
};

PyMODINIT_FUNC PyInit_libhfs(void)
{
    return PyModule_Create(&libhfs_module);
}
