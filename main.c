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

#define NAME_HFSVOL "hfsvol"
#define NAME_HFSDIR "hfsdir"
#define NAME_HFSFILE "hfsfile"

#define GETERR (hfs_error ? hfs_error : "unknown error")

static const char doc_mount[] =
    "mount(path, pnum, flags) -> hfsvol\n"
    "\n"
    "This routine attempts to open an HFS volume from a source pathname. The\n"
    "given `pnum' indicates which ordinal HFS partition is to be mounted,\n"
    "or can be 0 to indicate the entire medium should be mounted (ignoring\n"
    "any partition structure). If this value is not 0, the requested\n"
    "partition must exist.\n"
    "\n"
    "The `flags' argument specifies how the volume should be mounted.\n"
    "HFS_MODE_RDONLY means the volume should be mounted read-only.\n"
    "HFS_MODE_RDWR means the volume must be opened read/write. HFS_MODE_ANY\n"
    "means the volume can be mounted either read-only or read/write, with\n"
    "preference for the latter.\n"
    "\n"
    "The `flags' argument may also specify volume options. HFS_OPT_NOCACHE\n"
    "means not to perform any internal block caching, such as would be\n"
    "unnecessary for a volume residing in RAM, or if the associated overhead\n"
    "is not desired. HFS_OPT_ZERO means that newly-allocated blocks should be\n"
    "zero-initialized before use, primarily as a security feature for systems\n"
    "on which blocks may otherwise contain random data. Neither of these\n"
    "options should normally be necessary, and both may affect performance.\n"
    "\n"
    "An hfsvol object is returned. This object is used to access the volume\n"
    "and must eventually be passed to umount() to flush and close the\n"
    "volume and free all associated memory.";

static PyObject *wrap_mount(PyObject *self, PyObject *args)
{
    char *arg_path; int arg_pnum; int arg_flags;
    if(!PyArg_ParseTuple(args, "sii", &arg_path, &arg_pnum, &arg_flags))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    hfsvol *ret = hfs_mount(arg_path, arg_pnum, arg_flags);
    if(!ret)
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
   return PyCapsule_New((void *)ret, NAME_HFSVOL, NULL);
}

static const char doc_flush[] =
    "flush(hfsvol)\n"
    "\n"
    "This routine causes all pending changes to be flushed to an HFS volume.\n"
    "If a volume is kept open for a long period of time, it would be wise\n"
    "to call this periodically to avoid corrupting the volume due to\n"
    "unforeseen circumstances (power failure, floppy eject, etc.).";

static PyObject *wrap_flush(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_flush(arg_vol))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_flushall[] =
    "flushall()\n"
    "\n"
    "This routine is similar to flush() except that all mounted volumes\n"
    "are flushed, and errors are not reported.";

static PyObject *wrap_flushall(PyObject *self, PyObject *args)
{
    hfs_flushall();
    return Py_None;
}

static const char doc_umount[] =
    "umount(hfsvol)\n"
    "\n"
    "The specified HFS volume is unmounted; all open files and directories\n"
    "on the volume are closed, all pending changes to the volume are\n"
    "flushed, and all memory allocated for the volume is freed.\n"
    "\n"
    "All volumes opened mount() must eventually be closed with\n"
    "umount(), or they will risk corruption.\n"
    "\n"
    "The hfsvol object will become invalid, as will all objects\n"
    "representing open file or directory structures associated with\n"
    "the volume.";

static PyObject *wrap_umount(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_umount(arg_vol))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_umountall[] =
    "umountall()\n"
    "\n"
    "This routine is similar to umount() except that all mounted volumes\n"
    "are closed, and errors are not reported.\n"
    "\n"
    "This routine may be useful to call just before a process terminates to\n"
    "make sure any remaining open volumes are properly closed.";

static PyObject *wrap_umountall(PyObject *self, PyObject *args)
{
    hfs_umountall();
    return Py_None;
}

static const char doc_getvol[] =
    "getvol(name_bytes) -> hfsvol\n"
    "\n"
    "This routines searches all mounted volumes for one having the given\n"
    "`name_bytes', and returns its hfsvol object. If more than one\n"
    "volume have the same name, the most recently mounted one is returned.\n"
    "\n"
    "The given `name' is assumed to be encoded using MacOS Standard Roman.\n"
    "\n"
    "If an empty string is passed to this routine, the current volume is\n"
    "returned, if any.";

static PyObject *wrap_getvol(PyObject *self, PyObject *args)
{
    char *arg_vol;
    if(!PyArg_ParseTuple(args, "y", &arg_vol))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(!arg_vol[0]) arg_vol = NULL;
    hfsvol *ret = hfs_getvol(arg_vol);
    if(!ret)
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return PyCapsule_New((void *)ret, NAME_HFSVOL, NULL);
}

static const char doc_setvol[] =
    "setvol(hfsvol)\n"
    "\n"
    "The routine changes the \"current\" volume. Most HFS routines will accept\n"
    "a None hfsvol argument to mean the current volume; by default, the\n"
    "current volume is the last one which was mounted.";

static PyObject *wrap_setvol(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    hfs_setvol(arg_vol);
    return Py_None;
}

static const char doc_vstat[] =
    "vstat(hfsvol) -> ent\n"
    "\n"
    "This routine returns a volume entity structure `ent' with information\n"
    "about a mounted volume. The fields of the structure are defined in\n"
    "the hfs.h header file.";

static PyObject *wrap_vstat(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    hfsvolent ret_volent;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_vstat(arg_vol, &ret_volent))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_BuildValue("y#", (char *)(&ret_volent), sizeof(ret_volent));
}

static const char doc_vsetattr[] =
    "vsetattr(hfsvol, ent)\n"
    "\n"
    "This routine allows some attributes of a volume to be changed. The\n"
    "attributes which may be changed are: ent->clumpsz, ent->crdate,\n"
    "ent->mddate, ent->bkdate, and ent->blessed. Note that the default file\n"
    "clump size may only be changed to be a multiple of the volume's\n"
    "allocation block size, and the \"blessed\" folder must either be 0 or a\n"
    "valid folder CNID.\n"
    "\n"
    "To change the volume's name, use rename().";

static PyObject *wrap_vsetattr(PyObject *self, PyObject *args) // problems
{
    hfsvol *arg_vol; PyObject *arg_vol_c; hfsvolent *arg_ent; Py_ssize_t arg_ent_len;
    if(!PyArg_ParseTuple(args, "Oy#", &arg_vol_c, &arg_ent, &arg_ent_len))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_ent_len != sizeof(*arg_ent))
        {PyErr_SetString(PyExc_ValueError, "struct wrong len"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_vsetattr(arg_vol, arg_ent))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_chdir[] =
    "chdir(hfsvol, path_bytes)\n"
    "\n"
    "The \"current working directory\" for the given volume is changed.\n"
    "`path_bytes' can be either a relative or absolute HFS path.\n"
    "\n"
    "The given `path_bytes' is assumed to be encoded using MacOS Standard Roman.";

static PyObject *wrap_chdir(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_chdir(arg_vol, arg_path))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_getcwd[] =
    "getcwd(hfsvol) -> id\n"
    "\n"
    "The internal directory ID of the current working directory for the\n"
    "given volume is returned. This value is typically only useful for\n"
    "passing to setcwd() or dirinfo().";

static PyObject *wrap_getcwd(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c;
    if(!PyArg_ParseTuple(args, "O", &arg_vol_c))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    return Py_BuildValue("l", hfs_getcwd(arg_vol));
}

static const char doc_setcwd[] =
    "setcwd(hfsvol, id)\n"
    "\n"
    "This routine changes the current working directory for the given\n"
    "volume. A directory must exist with the given id.";

static PyObject *wrap_setcwd(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; long arg_id;
    if(!PyArg_ParseTuple(args, "Ol", &arg_vol_c, &arg_id))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_setcwd(arg_vol, arg_id))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_dirinfo[] =
    "dirinfo(hfsvol, id) -> parent_id, name_bytes\n"
    "\n"
    "This function looks up the given directory ID `id' and returns\n"
    "the directory ID of its parent. The name of the (child) directory\n"
    "is also returned.\n"
    "\n"
    "The string `name' will be encoded using MacOS Standard Roman.\n"
    "\n"
    "This function can be called repeatedly to construct a full pathname\n"
    "to the current working directory. The root directory of a volume\n"
    "always has a directory ID of HFS_CNID_ROOTDIR, and a pseudo-parent ID\n"
    "of HFS_CNID_ROOTPAR.";

static PyObject *wrap_dirinfo(PyObject *self, PyObject *args) // returns name in bytes object!
{
    hfsvol *arg_vol; PyObject *arg_vol_c; unsigned long argret_id;
    char ret_name[32];
    if(!PyArg_ParseTuple(args, "Ok", &arg_vol_c, &argret_id))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_dirinfo(arg_vol, &argret_id, ret_name))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_BuildValue("ly", argret_id, ret_name);
}

static const char doc_opendir[] =
    "opendir(hfsvol, path_bytes) -> hfsdir\n"
    "\n"
    "This function prepares to read the contents of a directory. `path_bytes'\n"
    "must be either an absolute or relative pathname to the desired HFS\n"
    "directory. As a special case, if `path' is an empty string, a\n"
    "\"meta-directory\" will be opened containing the root directories from\n"
    "all of the currently mounted volumes.\n"
    "\n"
    "The string `path_bytes' is assumed to be encoded using MacOS Standard Roman.\n"
    "\n"
    "This function returns an hfsdir object which must be passed to the other\n"
    "directory-related routines to read the directory.";

static PyObject *wrap_opendir(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    hfsdir *ret = hfs_opendir(arg_vol, arg_path);
    if(!ret)
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return PyCapsule_New((void *)ret, NAME_HFSDIR, NULL);
}

static const char doc_readdir[] =
    "readdir(hfsdir) -> ent\n"
    "\n"
    "This routine fills returns a directory entity structure `ent' with\n"
    "information about the next item in the given open directory. The\n"
    "fields of the structure are defined in the hfs.h header file.\n"
    "\n"
    "When no more items occur in the directory, this function returns None.";

static PyObject *wrap_readdir(PyObject *self, PyObject *args)
{
    hfsdir *arg_dir; PyObject *arg_dir_c;
    hfsdirent ret_ent;
    if(!PyArg_ParseTuple(args, "O", &arg_dir_c))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_dir_c == Py_None) arg_dir = NULL;
    else if(!(arg_dir = PyCapsule_GetPointer(arg_dir_c, NAME_HFSDIR)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSDIR); return NULL;}
    if(hfs_readdir(arg_dir, &ret_ent)) {
        if(errno == ENOENT) return Py_None;
        PyErr_SetString(PyExc_ValueError, GETERR); return NULL;
    }
    return Py_BuildValue("y#", (char *)(&ret_ent), sizeof(ret_ent));
}

static const char doc_closedir[] =
    "closedir(hfsdir)\n"
    "\n"
    "This function closes an open directory and frees all associated\n"
    "memory.\n"
    "\n"
    "The hfsdir object will no longer be valid.";

static PyObject *wrap_closedir(PyObject *self, PyObject *args)
{
    hfsdir *arg_dir; PyObject *arg_dir_c;
    if(!PyArg_ParseTuple(args, "O", &arg_dir_c))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_dir_c == Py_None) arg_dir = NULL;
    else if(!(arg_dir = PyCapsule_GetPointer(arg_dir_c, NAME_HFSDIR)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSDIR); return NULL;}
    if(hfs_closedir(arg_dir))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_create[] =
    "create(hfsvol, path_bytes, type_bytes, creator_bytes) -> hfsfile\n"
    "\n"
    "This routine creates a new, empty file with the given path, type, and\n"
    "creator. The type and creator must be strings of length 4, and have\n"
    "particular meaning under MacOS.\n"
    "\n"
    "The given `path_bytes' is assumed to be encoded using MacOS Standard Roman.\n"
    "\n"
    "The created file is opened and an hfsfile object is returned, the\n"
    "same as if open() had been called.";

static PyObject *wrap_create(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    char *arg_type; Py_ssize_t arg_type_len;
    char *arg_creator; Py_ssize_t arg_creator_len;
    if(!PyArg_ParseTuple(args, "Oyy#y#", &arg_vol_c, &arg_path, &arg_type, &arg_type_len, &arg_creator, &arg_creator_len))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_type_len != 4 || arg_creator_len != 4)
        {PyErr_SetString(PyExc_ValueError, "code wrong len"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    hfsfile *ret = hfs_create(arg_vol, arg_path, arg_type, arg_creator);
    if(!ret)
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return PyCapsule_New((void *)ret, NAME_HFSFILE, NULL);
}

static const char doc_open[] =
    "open(hfsvol, path_bytes) -> hfsfile\n"
    "\n"
    "This function opens an HFS file in preparation for I/O. Both forks of\n"
    "the file may be manipulated once the file is opened; setfork() is\n"
    "used to select the current fork. By default, the data fork is current.\n"
    "\n"
    "The given `path_bytes' is assumed to be encoded using MacOS Standard Roman.\n"
    "\n"
    "An hfsfile object is returned. This should be passed to other routines\n"
    "to manipulate the file.";

static PyObject *wrap_open(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    hfsfile *ret = hfs_open(arg_vol, arg_path);
    if(!ret)
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return PyCapsule_New((void *)ret, NAME_HFSFILE, NULL);
}

static const char doc_setfork[] =
    "setfork(hfsfile, fork)\n"
    "\n"
    "This routine selects the current fork in an open file for I/O. HFS\n"
    "files have two forks, data and resource. Resource forks normally contain\n"
    "structured data, although these HFS routines make no distinction\n"
    "between forks when reading or writing. It is up to higher-level\n"
    "applications to make sense of the information read or written from\n"
    "either fork.\n"
    "\n"
    "If 0 is passed to this routine, the data fork is selected. Otherwise\n"
    "the resource fork is selected. The seek pointer for the file is\n"
    "automatically reset to the beginning of the newly selected fork.\n"
    "\n"
    "As a side effect, this routine causes any excess disk blocks allocated\n"
    "for the fork which was current before the call to be freed; normally\n"
    "extra blocks are allocated during file writes to promote contiguity.\n"
    "The current fork will have been changed regardless of any error.";

static PyObject *wrap_setfork(PyObject *self, PyObject *args)
{
    hfsfile *arg_file; PyObject *arg_file_c; int arg_fork;
    if(!PyArg_ParseTuple(args, "Oi", &arg_file_c, &arg_fork))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSFILE); return NULL;}
    if(hfs_setfork(arg_file, arg_fork))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_getfork[] =
    "getfork(hfsfile) -> fork\n"
    "\n"
    "This routine returns an indication of which fork is currently active\n"
    "for I/O operations on the given file. If 0 is returned, the data fork\n"
    "is selected. Otherwise the resource fork is selected.\n"
    "";

static PyObject *wrap_getfork(PyObject *self, PyObject *args)
{
    hfsfile *arg_file; PyObject *arg_file_c;
    if(!PyArg_ParseTuple(args, "O", &arg_file_c))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSFILE); return NULL;}
    return Py_BuildValue("i", hfs_getfork(arg_file));
}

static const char doc_read[] =
    "read(hfsfile, bytearray)\n"
    "\n"
    "This routine tries to fill a bytearray with bytes from the current fork of an HFS\n"
    "file. The bytearray will be shortened to fit the number of bytes actually read\n"
    "if the end of the file is reached.\n"
    "\n"
    "It is most efficient to read data in multiples of HFS_BLOCKSZ byte\n"
    "blocks at a time.";

static PyObject *wrap_read(PyObject *self, PyObject *args) // pass in a bytearray and get it shrunk!
{
    hfsfile *arg_file; PyObject *arg_file_c; PyObject *arg_bytearray;
    if(!PyArg_ParseTuple(args, "OY", &arg_file_c, &arg_bytearray))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSFILE); return NULL;}
    long bytesread = hfs_read(arg_file, PyByteArray_AsString(arg_bytearray), PyByteArray_Size(arg_bytearray));
    if(bytesread == -1)
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    PyByteArray_Resize(arg_bytearray, bytesread);
    return Py_None;
}

static const char doc_write[] =
    "write(hfsfile, bytes) -> byteswritten\n"
    "\n"
    "This routine writes `bytes' to the current fork of an HFS file.\n"
    "The number of bytes actually written is returned.\n"
    "\n"
    "If the end of the file is reached before all bytes have been written,\n"
    "the file is automatically extended.\n"
    "\n"
    "It is most efficient to write data in multiples of HFS_BLOCKSZ byte\n"
    "blocks at a time.";

static PyObject *wrap_write(PyObject *self, PyObject *args) // pass in a bytearray and get it shrunk!
{
    hfsfile *arg_file; PyObject *arg_file_c; PyObject *arg_bytes; Py_ssize_t arg_bytes_len;
    if(!PyArg_ParseTuple(args, "Oy#", &arg_file_c, &arg_bytes, &arg_bytes_len))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSFILE); return NULL;}
    long byteswritten = hfs_write(arg_file, arg_bytes, arg_bytes_len);
    if(byteswritten == -1)
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_BuildValue("l", byteswritten);
}

static const char doc_truncate[] =
    "truncate(hfsfile, length)\n"
    "\n"
    "This routine causes the current fork of the specified open file to be\n"
    "truncated to at most `length' bytes.\n"
    "\n"
    "The disk blocks associated with the freed portion of the file are not\n"
    "actually deallocated until either the current fork is changed or the\n"
    "file is closed.";

static PyObject *wrap_truncate(PyObject *self, PyObject *args) // pass in a bytearray and get it shrunk!
{
    hfsfile *arg_file; PyObject *arg_file_c; unsigned long arg_len;
    if(!PyArg_ParseTuple(args, "Ok", &arg_file_c, &arg_len))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSFILE); return NULL;}
    if(hfs_truncate(arg_file, arg_len))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_seek[] =
    "seek(hfsfile, offset, from) -> location\n"
    "\n"
    "This routine changes the current seek pointer for the specified open\n"
    "file. This pointer determines where the next call to read() or\n"
    "write() will read or write data within the current fork.\n"
    "\n"
    "If `from' is HFS_SEEK_SET, the pointer is set to the absolute position\n"
    "given by `offset'.\n"
    "\n"
    "If `from' is HFS_SEEK_CUR, the pointer is offset from its current\n"
    "position by the amount `offset'. Positive offsets seek forward; negative\n"
    "offsets seek backward.\n"
    "\n"
    "If `from' is HFS_SEEK_END, the pointer is offset from the end of the\n"
    "file by the amount `offset', which ought not be positive.\n"
    "\n"
    "It is not presently possible to set the seek pointer beyond the logical\n"
    "end of the file.\n"
    "\n"
    "The new absolute position of the seek pointer is returned.";

static PyObject *wrap_seek(PyObject *self, PyObject *args) // pass in a bytearray and get it shrunk!
{
    hfsfile *arg_file; PyObject *arg_file_c; long arg_offset; int arg_from;
    if(!PyArg_ParseTuple(args, "Oli", &arg_file_c, &arg_offset, &arg_from))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSFILE); return NULL;}
    long absloc = hfs_seek(arg_file, arg_offset, arg_from);
    if(absloc == -1)
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_BuildValue("l", absloc);
}

static const char doc_close[] =
    "close(hfsfile)\n"
    "\n"
    "This routine causes all pending changes to the specified file to be\n"
    "flushed, and all storage associated with the file structure to be\n"
    "freed. Any excess disk blocks associated with the file are also\n"
    "deallocated at this time.\n"
    "\n"
    "The file structure pointer will no longer be valid.";

static PyObject *wrap_close(PyObject *self, PyObject *args) // pass in a bytearray and get it shrunk!
{
    hfsfile *arg_file; PyObject *arg_file_c;
    if(!PyArg_ParseTuple(args, "O", &arg_file_c))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSFILE); return NULL;}
    if(hfs_close(arg_file))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_stat[] =
    "stat(hfsvol, path_bytes) -> ent\n"
    "\n"
    "This routine returns a directory entity structure `ent' with\n"
    "information about the file or directory specified by `path_bytes' on the\n"
    "given volume. The fields of the structure are defined in the hfs.h\n"
    "header file.\n"
    "\n"
    "The given `path_bytes' is assumed to be encoded using MacOS Standard Roman.";

static PyObject *wrap_stat(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    hfsdirent ret_ent;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_stat(arg_vol, arg_path, &ret_ent))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_BuildValue("y#", (char *)(&ret_ent), sizeof(ret_ent));
}

static const char doc_fstat[] =
    "fstat(hfsfile) -> ent\n"
    "\n"
    "This routine is similar to stat() except it returns information\n"
    "about a file that is already open.";

static PyObject *wrap_fstat(PyObject *self, PyObject *args)
{
    hfsfile *arg_file; PyObject *arg_file_c;
    hfsdirent ret_ent;
    if(!PyArg_ParseTuple(args, "O", &arg_file_c))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSFILE); return NULL;}
    if(hfs_fstat(arg_file, &ret_ent))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_BuildValue("y#", (char *)(&ret_ent), sizeof(ret_ent));
}

static const char doc_setattr[] =
    "setattr(hfsvol, path_bytes, ent)\n"
    "\n"
    "This routine changes various attributes of an existing file or\n"
    "directory. The attributes which may be changed are: ent->crdate,\n"
    "ent->mddate, ent->bkdate, ent->fdflags, ent->fdlocation,\n"
    "ent->u.file.type, ent->u.file.creator, and ent->u.dir.rect. Also, the\n"
    "locked status of a file may be changed with ent->flags & HFS_ISLOCKED.\n"
    "\n"
    "The given `path_bytes' is assumed to be encoded using MacOS Standard Roman.";

static PyObject *wrap_setattr(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path; hfsdirent *arg_ent; Py_ssize_t arg_ent_len;
    if(!PyArg_ParseTuple(args, "Oyy#", &arg_vol_c, &arg_path, &arg_ent, &arg_ent_len))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_ent_len != sizeof(*arg_ent))
        {PyErr_SetString(PyExc_ValueError, "struct wrong len"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_setattr(arg_vol, arg_path, arg_ent))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_fsetattr[] =
    "fsetattr(hfsfile, ent)\n"
    "\n"
    "This routine is similar to setattr() except it manipulates a file\n"
    "that is already open.";

static PyObject *wrap_fsetattr(PyObject *self, PyObject *args)
{
    hfsfile *arg_file; PyObject *arg_file_c; hfsdirent *arg_ent; Py_ssize_t arg_ent_len;
    if(!PyArg_ParseTuple(args, "Oy#", &arg_file_c, &arg_ent, &arg_ent_len))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_ent_len != sizeof(*arg_ent))
        {PyErr_SetString(PyExc_ValueError, "struct wrong len"); return NULL;}
    if(arg_file_c == Py_None) arg_file = NULL;
    else if(!(arg_file = PyCapsule_GetPointer(arg_file_c, NAME_HFSFILE)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSFILE); return NULL;}
    if(hfs_fsetattr(arg_file, arg_ent))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_mkdir[] =
    "mkdir(hfsvol, path_bytes)\n"
    "\n"
    "This routine creates a new, empty directory with the given path_bytes.\n"
    "All parent directories must already exist, but there must not already\n"
    "be a file or directory with the complete given path.\n"
    "\n"
    "The given `path_bytes' is assumed to be encoded using MacOS Standard Roman.";

static PyObject *wrap_mkdir(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_mkdir(arg_vol, arg_path))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_rmdir[] =
    "rmdir(hfsvol, path_bytes)\n"
    "\n"
    "This routine deletes the directory with the given path. The directory\n"
    "must be empty.\n"
    "\n"
    "The given `pat_bytesh' is assumed to be encoded using MacOS Standard Roman.";

static PyObject *wrap_rmdir(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_rmdir(arg_vol, arg_path))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_delete[] =
    "delete(hfsvol, path_bytes)\n"
    "\n"
    "This routine deletes both forks of the file with the given path.\n"
    "\n"
    "The given `path_bytes' is assumed to be encoded using MacOS Standard Roman.";

static PyObject *wrap_delete(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_path;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_path))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_delete(arg_vol, arg_path))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_rename[] =
    "rename(hfsvol, srcpath_bytes, dstpath_bytes)\n"
    "\n"
    "This routine moves and/or renames the given `srcpath_bytes' to `dstpath_bytes'.\n"
    "The source must exist; the destination must not exist, unless it is a\n"
    "directory, in which case an attempt will be made to move the source\n"
    "into the destination directory without changing its name.\n"
    "\n"
    "If both `srcpath_bytes' and `dstpath_bytes' refer to root directories, the volume\n"
    "specified by `srcpath_bytes' will be renamed. Note that volume names may\n"
    "only have 1-27 (HFS_MAX_VLEN) characters, while all other names may\n"
    "have 1-31 (HFS_MAX_FLEN) characters.\n"
    "\n"
    "The given `srcpath_bytes' and `dstpath_bytes' are assumed to be encoded using MacOS\n"
    "Standard Roman.";

static PyObject *wrap_rename(PyObject *self, PyObject *args)
{
    hfsvol *arg_vol; PyObject *arg_vol_c; char *arg_srcpath; char *arg_dstpath;
    if(!PyArg_ParseTuple(args, "Oy", &arg_vol_c, &arg_srcpath, &arg_dstpath))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(arg_vol_c == Py_None) arg_vol = NULL;
    else if(!(arg_vol = PyCapsule_GetPointer(arg_vol_c, NAME_HFSVOL)))
        {PyErr_SetString(PyExc_ValueError, "bad " NAME_HFSVOL); return NULL;}
    if(hfs_rename(arg_vol, arg_srcpath, arg_dstpath))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_zero[] =
    "zero(path, maxparts) -> blocks\n"
    "\n"
    "This routine initializes a medium with a new, empty driver descriptor\n"
    "record and partition map. This is only necessary if it is desired to\n"
    "partition the medium; the medium can be used as a whole without\n"
    "partitions by specifying 0 to the routines which require a partition\n"
    "number.\n"
    "\n"
    "The partition map will be empty, with the exception of an entry for the\n"
    "partition map itself, plus an entry for the rest of the medium as free\n"
    "space. To be useful, one or more HFS partitions should be created with\n"
    "mkpart().\n"
    "\n"
    "The partition map will be created just large enough to allow `maxparts'\n"
    "individual partitions to be created, not counting the partitions created\n"
    "automatically by this routine. This number should be conservative, as\n"
    "it may be impossible to create more than this many partitions for the\n"
    "lifetime of the medium without re-initializing.\n"
    "\n"
    "The total number of blocks available for partitioning (after the\n"
    "partition map structures have been created) will be returned.";

static PyObject *wrap_zero(PyObject *self, PyObject *args)
{
    char *arg_path; unsigned int arg_maxparts;
    unsigned long ret_blocks;
    if(!PyArg_ParseTuple(args, "sI", &arg_path, &arg_maxparts))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(hfs_zero(arg_path, arg_maxparts, &ret_blocks))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_BuildValue("k", ret_blocks);
}

static const char doc_mkpart[] =
    "mkpart(path, length)\n"
    "\n"
    "This routine creates a new HFS partition having `length' blocks on the\n"
    "given medium. Space for the partition will be taken from the available\n"
    "free space as indicated in the existing partition map.\n"
    "\n"
    "It may not be possible to create the requested partition if there are\n"
    "not enough free contiguous blocks on the medium, or if there is only\n"
    "one slot left in the partition map and the request does not specify\n"
    "all the remaining blocks in the free space. (The partition map cannot\n"
    "leave any blocks in the medium unaccounted for.)";

static PyObject *wrap_mkpart(PyObject *self, PyObject *args)
{
    char *arg_path; unsigned long arg_len;
    if(!PyArg_ParseTuple(args, "sk", &arg_path, &arg_len))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(hfs_mkpart(arg_path, arg_len))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}

static const char doc_nparts[] =
    "nparts(path) -> num\n"
    "\n"
    "This routine determines the number of HFS partitions present on the\n"
    "given medium, if any. If the medium specified by `path' is not\n"
    "partitioned, -1 will be returned. Otherwise, a number denoting the total\n"
    "number of HFS partitions is returned, including (possibly) 0.\n"
    "\n"
    "The number returned by this routine can help determine if a particular\n"
    "medium is partitioned, and if so, the allowable range of partition\n"
    "numbers which can be passed to the routines which require one. However,\n"
    "passing 0 as a partition number always refers to the entire medium,\n"
    "ignoring all partitions.";

static PyObject *wrap_nparts(PyObject *self, PyObject *args)
{
    char *arg_path;
    int ret;
    if(!PyArg_ParseTuple(args, "s", &arg_path))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    ret = hfs_nparts(arg_path);
    if(ret == -1)
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_BuildValue("i", ret);
}

static const char doc_format[] =
    "format(path, pnum, mode, vname_bytes)\n"
    "\n"
    "This routine writes a new HFS file system to the specified `path', which\n"
    "should be a block device or a writable file. The size of the volume is\n"
    "determined either by the maximum size of the device or size of the file,\n"
    "or by the size of the indicated partition within the medium.\n"
    "\n"
    "If `pnum' is > 0, it selects an ordinal HFS partition in the device\n"
    "to receive the file system. The partition must already exist; an error\n"
    "will result if it cannot be found. With `pnum' == 0, any partition\n"
    "structure on the existing medium will be ignored, and the entire\n"
    "device will be used for the new HFS volume.\n"
    "\n"
    "Volume options may be specified in the `mode' argument. In addition to\n"
    "the options accepted by mount(), HFS_OPT_2048 may be specified to\n"
    "request that the volume allocation blocks be aligned on physical\n"
    "2048-byte block boundaries. Such a constraint is necessary to support\n"
    "some hybrid CD-ROM file system formats, but is otherwise unnecessary and\n"
    "may result in fewer allocation blocks altogether.\n"
    "\n"
    "The volume is given the name `vname_bytes', which must be between 1 and\n"
    "HFS_MAX_VLEN (27) characters in length inclusively, and cannot contain\n"
    "any colons (':'). This string is assumed to be encoded using MacOS\n"
    "Standard Roman.\n"
    "\n"
    "UNIMPLEMENTED:\n"
    "It is possible to map out or \"spare\" bad blocks on the device such that\n"
    "the file system will be made aware of these blocks and will not attempt\n"
    "to use them to store data. To perform this magic, format() may be\n"
    "passed an array of block numbers to spare. These numbers must\n"
    "correspond to logical 512-byte blocks on the device and should be\n"
    "relative to the beginning of the volume's partition, if any. If no\n"
    "blocks need to be spared, 0 should be passed for `nbadblocks', and\n"
    "`badblocks' may be a NULL pointer. Note that an error can occur if a\n"
    "bad block occurs in a critical disk structure, or if there are too\n"
    "many bad blocks (more than 25%) in the volume.";

static PyObject *wrap_format(PyObject *self, PyObject *args) // bad blocks unimplemented
{
    char *arg_path; int arg_pnum; int arg_mode; char *arg_vname;
    if(!PyArg_ParseTuple(args, "siiy", &arg_path, &arg_pnum, &arg_mode, &arg_vname))
        {PyErr_SetString(PyExc_ValueError, "bad args"); return NULL;}
    if(hfs_format(arg_path, arg_pnum, arg_mode, arg_vname, 0, NULL))
        {PyErr_SetString(PyExc_ValueError, GETERR); return NULL;}
    return Py_None;
}


static PyMethodDef module_methods[] = {
// Volume routines
    {"mount", wrap_mount, METH_VARARGS, doc_mount},
    {"flush", wrap_flush, METH_VARARGS, doc_flush},
    {"flushall", wrap_flushall, METH_NOARGS, doc_flushall},
    {"umount", wrap_umount, METH_VARARGS, doc_umount},
    {"umountall", wrap_umountall, METH_NOARGS, doc_umountall},
    {"getvol", wrap_getvol, METH_VARARGS, doc_getvol},
    {"setvol", wrap_setvol, METH_VARARGS, doc_setvol},
    {"vstat", wrap_vstat, METH_VARARGS, doc_vstat},
    {"vsetattr", wrap_vsetattr, METH_VARARGS, doc_vsetattr},
// Directory routines
    {"chdir", wrap_chdir, METH_VARARGS, doc_chdir},
    {"getcwd", wrap_getcwd, METH_VARARGS, doc_getcwd},
    {"setcwd", wrap_setcwd, METH_VARARGS, doc_setcwd},
    {"dirinfo", wrap_dirinfo, METH_VARARGS, doc_dirinfo},
    {"opendir", wrap_opendir, METH_VARARGS, doc_opendir},
    {"readdir", wrap_readdir, METH_VARARGS, doc_readdir},
    {"closedir", wrap_closedir, METH_VARARGS, doc_closedir},
// File routines
    {"create", wrap_create, METH_VARARGS, doc_create},
    {"open", wrap_open, METH_VARARGS, doc_open},
    {"setfork", wrap_setfork, METH_VARARGS, doc_setfork},
    {"getfork", wrap_getfork, METH_VARARGS, doc_getfork},
    {"read", wrap_read, METH_VARARGS, doc_read},
    {"write", wrap_write, METH_VARARGS, doc_write},
    {"truncate", wrap_truncate, METH_VARARGS, doc_truncate},
    {"seek", wrap_seek, METH_VARARGS, doc_seek},
    {"close", wrap_close, METH_VARARGS, doc_close},
// Catalog routines
    {"stat", wrap_stat, METH_VARARGS, doc_stat},
    {"fstat", wrap_fstat, METH_VARARGS, doc_fstat},
    {"setattr", wrap_setattr, METH_VARARGS, doc_setattr},
    {"fsetattr", wrap_fsetattr, METH_VARARGS, doc_fsetattr},
    {"mkdir", wrap_mkdir, METH_VARARGS, doc_mkdir},
    {"rmdir", wrap_rmdir, METH_VARARGS, doc_rmdir},
    {"delete", wrap_delete, METH_VARARGS, doc_delete},
    {"rename", wrap_rename, METH_VARARGS, doc_rename},
// Media routines
    {"zero", wrap_zero, METH_VARARGS, doc_zero},
    {"mkpart", wrap_mkpart, METH_VARARGS, doc_mkpart},
    {"nparts", wrap_nparts, METH_VARARGS, doc_nparts},
    {"format", wrap_format, METH_VARARGS, doc_format},

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
