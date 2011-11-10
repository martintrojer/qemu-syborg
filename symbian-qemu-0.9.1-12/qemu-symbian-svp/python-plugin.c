/*
*
* Contributors:
* NTT DOCOMO, INC. -- Syborg QEMU crashes when using skin + touchscreen device
* 
* Nokia Oyj -- New memregion service added for converting guest OS addresses to host OS addresses
*
*/

#include "Python.h"
#include "structmember.h"
#include "hw/hw.h"
#include "sysemu.h"
#include "devtree.h"
#include "qemu-char.h"
#include "display_state.h"
#include "hw/gui.h"
#include "hw/fb_render_engine.h"
#include "qemu-timer.h"

static void qemu_py_die(void)
{
    PyErr_Print();
    exit(1);
}

#define qemu_py_assert(x) do { if (!(x)) { qemu_py_die(); } } while (0)

static void qemu_py_load_module(const char *name)
{
    PyObject *module;
    module = PyImport_ImportModule(name);
    if (!module)
        qemu_py_die();
}

static int64_t qemu_py_int64_from_pynum(PyObject *ob)
{
    if (PyInt_Check(ob))
        return PyInt_AsLong(ob);
    return PyLong_AsLongLong(ob);
}

static uint64_t qemu_py_uint64_from_pynum(PyObject *ob)
{
    if (PyInt_Check(ob))
        return PyInt_AsUnsignedLongMask(ob);
    return PyLong_AsUnsignedLongLongMask(ob);
}

typedef struct {
    PyObject_HEAD
    PyObject *size;
    PyObject *readl;
    PyObject *writel;
} qemu_py_ioregion;

static void qemu_py_ioregion_dealloc(qemu_py_ioregion *self)
{
    Py_CLEAR(self->size);
    Py_CLEAR(self->readl);
    Py_CLEAR(self->writel);
    self->ob_type->tp_free((PyObject*)self);
}

static int qemu_py_ioregion_init(qemu_py_ioregion *self, PyObject *args,
                                 PyObject *kwds)
{
    static char *kwlist[] = {"size", "readl", "writel", NULL};
    PyObject *obsize;
    PyObject *readl;
    PyObject *writel;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist,
                                     &obsize, &readl, &writel))
        return -1; 

    Py_INCREF(obsize);
    self->size = obsize;
    Py_INCREF(readl);
    self->readl = readl;
    Py_INCREF(writel);
    self->writel = writel;

    return 0;
}

static PyMemberDef qemu_py_ioregion_members[] = {
    {"size", T_OBJECT, offsetof(qemu_py_ioregion, size), 0,
     "size"},
    {"readl", T_OBJECT, offsetof(qemu_py_ioregion, readl), 0,
     "32-bit read"},
    {"writel", T_OBJECT, offsetof(qemu_py_ioregion, writel), 0,
     "32-bit write"},
    {NULL}  /* Sentinel */
};

static PyTypeObject qemu_py_ioregionType = {
    PyObject_HEAD_INIT(NULL)
    0,                                    /* ob_size */
    "qemu.ioregion",                      /* tp_name */
    sizeof(qemu_py_ioregion),             /* tp_basicsize */
    0,                                    /* tp_itemsize */
    (destructor)qemu_py_ioregion_dealloc, /* tp_dealloc */
    0,                                    /* tp_print */
    0,                                    /* tp_getattr */
    0,                                    /* tp_setattr */
    0,                                    /* tp_compare */
    0,                                    /* tp_repr */
    0,                                    /* tp_as_number */
    0,                                    /* tp_as_sequence */
    0,                                    /* tp_as_mapping */
    0,                                    /* tp_hash  */
    0,                                    /* tp_call */
    0,                                    /* tp_str */
    0,                                    /* tp_getattro */
    0,                                    /* tp_setattro */
    0,                                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                   /* tp_flags */
    "QEMU IORegion",                      /* tp_doc */
    0,                                    /* tp_traverse */
    0,                                    /* tp_clear */
    0,		                          /* tp_richcompare */
    0,		                          /* tp_weaklistoffset */
    0,		                          /* tp_iter */
    0,		                          /* tp_iternext */
    0,                                    /* tp_methods */
    qemu_py_ioregion_members,             /* tp_members */
    0,                                    /* tp_getset */
    0,                                    /* tp_base */
    0,                                    /* tp_dict */
    0,                                    /* tp_descr_get */
    0,                                    /* tp_descr_set */
    0,                                    /* tp_dictoffset */
    (initproc)qemu_py_ioregion_init,      /* tp_init */
    0,                                    /* tp_alloc */
    0,                                    /* tp_new */
};

typedef struct {
    PyObject_HEAD
    uint32_t base;
    uint32_t size;
	uint32_t *host_ram_base_ptr;
} qemu_py_memregion;

static void qemu_py_memregion_dealloc(qemu_py_ioregion *self)
	{
    self->ob_type->tp_free((PyObject*)self);
	}

static int qemu_py_memregion_init(qemu_py_memregion *self, PyObject *args,
                                 PyObject *kwds)
	{
    static char *kwlist[] = {"base","size", NULL};
    uint32_t base;
    uint32_t size;
	int ret = -1;
	int region_index = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ll", kwlist,
                                     &base, &size ))
		{
        ret = -1; 
		}
	else
		{
		for( region_index = 0; region_index < devtree_ram_map_size; region_index+=1 )
			{
			// The created region needs to fit inside memory regions in device tree configuration
			if( (devtree_ram_map[region_index].base <= base) && 
				((devtree_ram_map[region_index].base + devtree_ram_map[region_index].size) >=
				(base + size)) )
				{
				ret = 0;
				self->base = base;
				self->size = size;
				self->host_ram_base_ptr = (uint32_t*)host_ram_addr(base);
				break;
				}
			else
				{
				ret = -1;
				}
			}
		}
	
    return ret;
	}

static PyObject *qemu_py_memregion_get_size(qemu_py_memregion *self,
                                    PyObject *args, PyObject *kwds)
	{
	return PyLong_FromUnsignedLong(self->size);
	}

static PyObject *qemu_py_memregion_get_base(qemu_py_memregion *self,
                                    PyObject *args, PyObject *kwds)
	{
	return PyLong_FromUnsignedLong(self->base);
	}

static PyObject *qemu_py_memregion_get_host_base(qemu_py_memregion *self,
                                    PyObject *args, PyObject *kwds)
	{
	return PyLong_FromVoidPtr( (void*)(self->host_ram_base_ptr) );
	}

static PyMethodDef qemu_py_memregion_methods[] = {
    {"size", (PyCFunction)qemu_py_memregion_get_size, METH_VARARGS|METH_KEYWORDS,
     "Get memory region size"},
    {"region_guest_addr", (PyCFunction)qemu_py_memregion_get_base, METH_VARARGS|METH_KEYWORDS,
     "Get memory region base"},
    {"region_host_addr", (PyCFunction)qemu_py_memregion_get_host_base, METH_VARARGS|METH_KEYWORDS,
     "Get memory region host base address"},
    {NULL}  /* Sentinel */
};

static PyTypeObject qemu_py_memRegionType = {
    PyObject_HEAD_INIT(NULL)
    0,                                      /* ob_size */
    "qemu.memregion",                       /* tp_name */
    sizeof(qemu_py_memregion),              /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor)qemu_py_memregion_dealloc,  /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    0,                                      /* tp_hash  */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    0,                                      /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                     /* tp_flags */
    "QEMU MemRegion",                       /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,		                                /* tp_richcompare */
    0,		                                /* tp_weaklistoffset */
    0,		                                /* tp_iter */
    0,		                                /* tp_iternext */
    qemu_py_memregion_methods,              /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)qemu_py_memregion_init,       /* tp_init */
    0,                                      /* tp_alloc */
    0,                                      /* tp_new */
};

typedef struct {
    PyObject_HEAD
    QEMUFile *f;
} qemu_py_file;

static void qemu_py_file_dealloc(qemu_py_file *self)
{
    self->f = NULL;
    self->ob_type->tp_free((PyObject*)self);
}

static int qemu_py_file_init(qemu_py_file *self, PyObject *args,
                             PyObject *kwds)
{
    static char *kwlist[] = {"f", NULL};
    PyObject *obdata;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &obdata))
        return -1; 

    self->f = PyCObject_AsVoidPtr(obdata);
    if (PyErr_Occurred())
        return -1;

    return 0;
}

static PyObject *qemu_py_file_get_u32(qemu_py_file *self, PyObject *args)
{
    uint32_t val;

    if (self->f) {
        val = qemu_get_be32(self->f);
        return PyLong_FromUnsignedLong(val);
    }
    Py_RETURN_NONE;
}

static PyObject *qemu_py_file_put_u32(qemu_py_file *self,
                                      PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", NULL};
    PyObject *obval;
    uint32_t val;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &obval))
        return NULL;

    val = qemu_py_uint64_from_pynum(obval);
    if (PyErr_Occurred())
        return NULL;

    if (self->f)
        qemu_put_be32(self->f, val);

    Py_RETURN_NONE;
}

static PyObject *qemu_py_file_get_u64(qemu_py_file *self, PyObject *args)
{
    uint64_t val;

    if (self->f) {
        val = qemu_get_be64(self->f);
        return PyLong_FromUnsignedLongLong(val);
    }
    Py_RETURN_NONE;
}

static PyObject *qemu_py_file_get_s64(qemu_py_file *self, PyObject *args)
{
    int64_t val;

    if (self->f) {
        val = qemu_get_be64(self->f);
        return PyLong_FromLongLong(val);
    }
    Py_RETURN_NONE;
}

static PyObject *qemu_py_file_put_u64(qemu_py_file *self,
                                      PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", NULL};
    PyObject *obval;
    uint64_t val;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &obval))
        return NULL;

    val = qemu_py_uint64_from_pynum(obval);
    if (PyErr_Occurred())
        return NULL;

    if (self->f)
        qemu_put_be64(self->f, val);

    Py_RETURN_NONE;
}

static PyObject *qemu_py_file_put_s64(qemu_py_file *self,
                                      PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", NULL};
    PyObject *obval;
    int64_t val;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &obval))
        return NULL;

    val = qemu_py_int64_from_pynum(obval);
    if (PyErr_Occurred())
        return NULL;

    if (self->f)
        qemu_put_be64(self->f, val);

    Py_RETURN_NONE;
}

static PyMethodDef qemu_py_file_methods[] = {
    {"get_u32", (PyCFunction)qemu_py_file_get_u32, METH_NOARGS,
     "Read unsigned 32-bit value"},
    {"put_u32", (PyCFunction)qemu_py_file_put_u32, METH_VARARGS|METH_KEYWORDS,
     "Write unsigned 32-bit value"},
    {"get_u64", (PyCFunction)qemu_py_file_get_u64, METH_NOARGS,
     "Read unsigned 64-bit value"},
    {"put_u64", (PyCFunction)qemu_py_file_put_u64, METH_VARARGS|METH_KEYWORDS,
     "Write unsigned 64-bit value"},
    {"get_s64", (PyCFunction)qemu_py_file_get_s64, METH_NOARGS,
     "Read signed 64-bit value"},
    {"put_s64", (PyCFunction)qemu_py_file_put_s64, METH_VARARGS|METH_KEYWORDS,
     "Write signed 64-bit value"},
    {NULL}  /* Sentinel */
};

static PyTypeObject qemu_py_fileType = {
    PyObject_HEAD_INIT(NULL)
    0,                                    /* ob_size */
    "qemu.file",                          /* tp_name */
    sizeof(qemu_py_file),                 /* tp_basicsize */
    0,                                    /* tp_itemsize */
    (destructor)qemu_py_file_dealloc,     /* tp_dealloc */
    0,                                    /* tp_print */
    0,                                    /* tp_getattr */
    0,                                    /* tp_setattr */
    0,                                    /* tp_compare */
    0,                                    /* tp_repr */
    0,                                    /* tp_as_number */
    0,                                    /* tp_as_sequence */
    0,                                    /* tp_as_mapping */
    0,                                    /* tp_hash  */
    0,                                    /* tp_call */
    0,                                    /* tp_str */
    0,                                    /* tp_getattro */
    0,                                    /* tp_setattro */
    0,                                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                   /* tp_flags */
    "QEMU File Object",                   /* tp_doc */
    0,                                    /* tp_traverse */
    0,                                    /* tp_clear */
    0,		                          /* tp_richcompare */
    0,		                          /* tp_weaklistoffset */
    0,		                          /* tp_iter */
    0,		                          /* tp_iternext */
    qemu_py_file_methods,                 /* tp_methods */
    0,                                    /* tp_members */
    0,                                    /* tp_getset */
    0,                                    /* tp_base */
    0,                                    /* tp_dict */
    0,                                    /* tp_descr_get */
    0,                                    /* tp_descr_set */
    0,                                    /* tp_dictoffset */
    (initproc)qemu_py_file_init,          /* tp_init */
    0,                                    /* tp_alloc */
    0,                                    /* tp_new */
};

typedef struct {
    PyObject_HEAD
    ptimer_state *ptimer;
} qemu_py_ptimer;

static void qemu_py_ptimer_dealloc(qemu_py_ptimer *self)
{
    self->ptimer = NULL;
    self->ob_type->tp_free((PyObject*)self);
}

static void qemu_py_ptimer_tick(void *opaque)
{
    PyObject *fn = opaque;
    PyObject *obval;

    obval = PyObject_CallFunctionObjArgs(fn, NULL);
    qemu_py_assert(obval);
    Py_DECREF(obval);
}

static int qemu_py_ptimer_init(qemu_py_ptimer *self, PyObject *args,
                               PyObject *kwds)
{
    static char *kwlist[] = {"tick", "freq", NULL};
    PyObject *tick;
    PyObject *obfreq = NULL;
    uint64_t freq;
    QEMUBH *bh;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist,
                                     &tick, &obfreq))
        return -1; 

    if (!PyCallable_Check(tick)) {
        PyErr_SetString(PyExc_TypeError, "tick handler must be callable");
        return -1;
    }
    if (obfreq) {
        freq = qemu_py_uint64_from_pynum(obfreq);
        if (PyErr_Occurred())
            return -1;
    } else {
        freq = 0;
    }
    Py_INCREF(tick);
    bh = qemu_bh_new(qemu_py_ptimer_tick, tick);
    self->ptimer = ptimer_init(bh);
    if (freq)
        ptimer_set_freq(self->ptimer, freq);

    return 0;
}

static PyObject *qemu_py_ptimer_run(qemu_py_ptimer *self,
                                    PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"oneshot", NULL};
    int oneshot;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &oneshot))
        return NULL;

    ptimer_run(self->ptimer, oneshot != 0);
    Py_RETURN_NONE;
}

static PyObject *qemu_py_ptimer_stop(qemu_py_ptimer *self, PyObject *args)
{
    ptimer_stop(self->ptimer);
    Py_RETURN_NONE;
}

static PyObject *qemu_py_ptimer_set_limit(qemu_py_ptimer *self,
                                          PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"limit", "reload", NULL};
    PyObject *oblimit;
    uint64_t limit;
    int reload;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oi", kwlist, &oblimit,
                                     &reload))
        return NULL;

    limit = qemu_py_uint64_from_pynum(oblimit);
    if (PyErr_Occurred())
        return NULL;

    ptimer_set_limit(self->ptimer, limit, reload != 0);

    Py_RETURN_NONE;
}

static PyObject *qemu_py_ptimer_get(qemu_py_ptimer *self,
                                    PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"f", NULL};
    qemu_py_file *f;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist,
                                     &qemu_py_fileType, &f))
        return NULL;

    qemu_get_ptimer(f->f, self->ptimer);

    Py_RETURN_NONE;
}

static PyObject *qemu_py_ptimer_put(qemu_py_ptimer *self,
                                    PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"f", NULL};
    qemu_py_file *f;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist,
                                     &qemu_py_fileType, &f))
        return NULL;

    qemu_put_ptimer(f->f, self->ptimer);

    Py_RETURN_NONE;
}

static PyObject *qemu_py_ptimer_get_count(qemu_py_ptimer *self, void *closure)
{
    return PyLong_FromUnsignedLongLong(ptimer_get_count(self->ptimer));
}

static int qemu_py_ptimer_set_count(qemu_py_ptimer *self, PyObject *obval,
                                    void *closure)
{
    uint64_t val;
    if (obval == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete ptimer count");
        return -1;
    }

    val = qemu_py_uint64_from_pynum(obval);
    if (PyErr_Occurred())
        return -1;

    ptimer_set_count(self->ptimer, val);

    return 0;
}

static PyGetSetDef qemu_py_ptimer_getseters [] = {
    {"count", (getter)qemu_py_ptimer_get_count,
     (setter)qemu_py_ptimer_set_count,
     "current counter value", NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef qemu_py_ptimer_methods[] = {
    {"run", (PyCFunction)qemu_py_ptimer_run, METH_VARARGS|METH_KEYWORDS,
     "Start timer"},
    {"stop", (PyCFunction)qemu_py_ptimer_stop, METH_NOARGS,
     "Stop timer"},
    {"set_limit", (PyCFunction)qemu_py_ptimer_set_limit,
     METH_VARARGS|METH_KEYWORDS,
     "Set reload value"},
    {"get", (PyCFunction)qemu_py_ptimer_get, METH_VARARGS|METH_KEYWORDS,
     "Restore state"},
    {"put", (PyCFunction)qemu_py_ptimer_put, METH_VARARGS|METH_KEYWORDS,
     "Save state"},
    {NULL}  /* Sentinel */
};

static PyTypeObject qemu_py_ptimerType = {
    PyObject_HEAD_INIT(NULL)
    0,                                    /* ob_size */
    "qemu.ptimer",                        /* tp_name */
    sizeof(qemu_py_ptimer),               /* tp_basicsize */
    0,                                    /* tp_itemsize */
    (destructor)qemu_py_ptimer_dealloc,   /* tp_dealloc */
    0,                                    /* tp_print */
    0,                                    /* tp_getattr */
    0,                                    /* tp_setattr */
    0,                                    /* tp_compare */
    0,                                    /* tp_repr */
    0,                                    /* tp_as_number */
    0,                                    /* tp_as_sequence */
    0,                                    /* tp_as_mapping */
    0,                                    /* tp_hash  */
    0,                                    /* tp_call */
    0,                                    /* tp_str */
    0,                                    /* tp_getattro */
    0,                                    /* tp_setattro */
    0,                                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                   /* tp_flags */
    "QEMU Periodic Timer",                /* tp_doc */
    0,                                    /* tp_traverse */
    0,                                    /* tp_clear */
    0,		                          /* tp_richcompare */
    0,		                          /* tp_weaklistoffset */
    0,		                          /* tp_iter */
    0,		                          /* tp_iternext */
    qemu_py_ptimer_methods,               /* tp_methods */
    0,                                    /* tp_members */
    qemu_py_ptimer_getseters,             /* tp_getset */
    0,                                    /* tp_base */
    0,                                    /* tp_dict */
    0,                                    /* tp_descr_get */
    0,                                    /* tp_descr_set */
    0,                                    /* tp_dictoffset */
    (initproc)qemu_py_ptimer_init,        /* tp_init */
    0,                                    /* tp_alloc */
    0,                                    /* tp_new */
};

typedef struct {
    PyObject_HEAD
    render_data *rdata;
} qemu_py_palette;

static void qemu_py_palette_dealloc(qemu_py_palette *self)
{
    self->rdata = NULL;
    self->ob_type->tp_free((PyObject*)self);
}

static int qemu_py_palette_init(qemu_py_palette *self, PyObject *args,
                                PyObject *kwds)
{
    static char *kwlist[] = {"rdata", NULL};
    PyObject *obdata;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &obdata))
        return -1; 

    self->rdata = PyCObject_AsVoidPtr(obdata);
    if (PyErr_Occurred())
        return -1;

    return 0;
}

static Py_ssize_t qemu_py_palette_len(PyObject *o)
{
    return 256;
}

static PyObject *qemu_py_palette_getitem(PyObject *o, Py_ssize_t i)
{
    qemu_py_palette *self = (qemu_py_palette *)o;

    if (i < 0 || i >= 256) {
        PyErr_SetString(PyExc_ValueError, "Palette index must be 0-255");
        return NULL;
    }

    return PyLong_FromUnsignedLong(get_palette_value(self->rdata, i));
}

static int qemu_py_palette_setitem(PyObject *o, Py_ssize_t i, PyObject *obval)
{
    qemu_py_palette *self = (qemu_py_palette *)o;
    uint32_t val;

    if (i < 0 || i >= 256) {
        PyErr_SetString(PyExc_ValueError, "Palette index must be 0-255");
        return -1;
    }

    val = PyLong_AsUnsignedLongMask(obval);
    if (PyErr_Occurred())
        return -1;
    set_palette_value(self->rdata, i, val);
    return 0;
}

static PySequenceMethods qemu_py_palette_seq = {
      .sq_length = qemu_py_palette_len,
      .sq_item = qemu_py_palette_getitem,
      .sq_ass_item = qemu_py_palette_setitem
};

static PyTypeObject qemu_py_paletteType = {
    PyObject_HEAD_INIT(NULL)
    0,                                    /* ob_size */
    "qemu.palette",                       /* tp_name */
    sizeof(qemu_py_palette),              /* tp_basicsize */
    0,                                    /* tp_itemsize */
    (destructor)qemu_py_palette_dealloc,  /* tp_dealloc */
    0,                                    /* tp_print */
    0,                                    /* tp_getattr */
    0,                                    /* tp_setattr */
    0,                                    /* tp_compare */
    0,                                    /* tp_repr */
    0,                                    /* tp_as_number */
    &qemu_py_palette_seq,                 /* tp_as_sequence */
    0,                                    /* tp_as_mapping */
    0,                                    /* tp_hash  */
    0,                                    /* tp_call */
    0,                                    /* tp_str */
    0,                                    /* tp_getattro */
    0,                                    /* tp_setattro */
    0,                                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                   /* tp_flags */
    "QEMU Framebuffer Palette",           /* tp_doc */
    0,                                    /* tp_traverse */
    0,                                    /* tp_clear */
    0,		                          /* tp_richcompare */
    0,		                          /* tp_weaklistoffset */
    0,		                          /* tp_iter */
    0,		                          /* tp_iternext */
    0,                                    /* tp_methods */
    0,                                    /* tp_members */
    0,                                    /* tp_getset */
    0,                                    /* tp_base */
    0,                                    /* tp_dict */
    0,                                    /* tp_descr_get */
    0,                                    /* tp_descr_set */
    0,                                    /* tp_dictoffset */
    (initproc)qemu_py_palette_init,       /* tp_init */
    0,                                    /* tp_alloc */
    0,                                    /* tp_new */
};

typedef struct {
    PyObject_HEAD
    render_data *rdata;
    DisplayState *ds;
    PyObject *update_cb;
    PyObject *palette;
    int need_update;
} qemu_py_render;

static void qemu_py_render_dealloc(qemu_py_render *self)
{
    if (self->rdata)
        destroy_render_data(self->rdata);
    self->rdata = NULL;
    Py_CLEAR(self->palette);
    self->ob_type->tp_free((PyObject*)self);
}

static void qemu_py_update_display(void *opaque)
{
    qemu_py_render *self = opaque;
    PyObject *obval;
    PyObject *fn;
    int do_update;

    if (nographic || ds_get_bits_per_pixel(self->ds) == 0)
        return;

    fn = self->update_cb;

    if (fn && PyCallable_Check(fn)) {
        obval = PyObject_CallFunctionObjArgs(fn, NULL);
        qemu_py_assert(obval);
        do_update = PyInt_AsLong(obval);
        qemu_py_assert(!PyErr_Occurred());
        Py_DECREF(obval);

        if (!do_update)
            return;
    }

    render(self->ds, self->rdata, self->need_update);
    self->need_update = 0;
}

static void qemu_py_invalidate_display(void *opaque)
{
    qemu_py_render *self = opaque;
    self->need_update = 1;
}

static int qemu_py_render_init(qemu_py_render *self, PyObject *args,
                               PyObject *kwds)
{
    static char *kwlist[] = {"name", "width", "height", NULL};
    int width;
    int height;
    char *name;
    PyObject *ob;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sii", kwlist,
                                     &name, &width, &height))
        return -1; 

    self->ds = gui_get_graphic_console(name,
                                       qemu_py_update_display,
                                       qemu_py_invalidate_display,
                                       NULL, self);

    if (!self->ds) {
        PyErr_SetString(PyExc_AssertionError, "Display creation failed");
        return -1;
    }

    if (width && height) {
        if (!gui_resize_vt(self->ds, width, height)) {
            /* can't resize */
            width = height = 0;
        }
    }
    if (!width)
        width = ds_get_width(self->ds);
    if (!height)
        height = ds_get_height(self->ds);

    self->rdata = create_render_data();
    set_cols(self->rdata, width);
    set_rows(self->rdata, height);

    ob = PyCObject_FromVoidPtr(self->rdata, NULL);
    if (!ob)
        return -1;
    self->palette =
      PyObject_CallFunctionObjArgs((PyObject *)&qemu_py_paletteType,
                                   ob, NULL);
    if (!self->palette)
        return -1;
    Py_DECREF(ob);
    return 0;
}

static PyObject *qemu_py_render_get(qemu_py_render *self,
                                    PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"f", NULL};
    qemu_py_file *f;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist,
                                     &qemu_py_fileType, &f))
        return NULL;

    qemu_get_render_data(f->f, self->rdata);

    Py_RETURN_NONE;
}

static PyObject *qemu_py_render_put(qemu_py_render *self,
                                    PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"f", NULL};
    qemu_py_file *f;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist,
                                     &qemu_py_fileType, &f))
        return NULL;

    qemu_put_render_data(f->f, self->rdata);

    Py_RETURN_NONE;
}

static PyMemberDef qemu_py_render_members[] = {
    {"update", T_OBJECT, offsetof(qemu_py_render, update_cb), 0,
     "Display update callback"},
    {"palette", T_OBJECT, offsetof(qemu_py_render, palette), 0,
     "palette"},
    {NULL}  /* Sentinel */
};

enum {
    QEMU_PY_FB_BASE,
    QEMU_PY_FB_WIDTH,
    QEMU_PY_FB_HEIGHT,
    QEMU_PY_FB_ORIENTATION,
    QEMU_PY_FB_BLANK,
    QEMU_PY_FB_BPP,
    QEMU_PY_FB_COLOR_ORDER,
    QEMU_PY_FB_BYTE_ORDER,
    QEMU_PY_FB_PIXEL_ORDER,
    QEMU_PY_FB_ROW_PITCH
};

static PyObject *
qemu_py_render_getattr(qemu_py_render *self, void *closure)
{
    /* FIXME: should be target_phys_addr_t?  */
    uint32_t val;
    render_data *rdata = self->rdata;

    switch ((size_t)closure) {
    case QEMU_PY_FB_BASE:
        val = get_fb_base_in_target(rdata);
        break;
    case QEMU_PY_FB_WIDTH:
        val = get_cols(rdata);
        break;
    case QEMU_PY_FB_HEIGHT:
        val = get_rows(rdata);
        break;
    case QEMU_PY_FB_ORIENTATION:
        val = get_orientation(rdata);
        break;
    case QEMU_PY_FB_BLANK:
        val = get_blank_mode(rdata);
        break;
    case QEMU_PY_FB_BPP:
        switch (get_src_bpp(rdata)) {
        case BPP_SRC_1: val = 1; break;
        case BPP_SRC_2: val = 2; break;
        case BPP_SRC_4: val = 4; break;
        case BPP_SRC_8: val = 8; break;
        case BPP_SRC_15: val = 15; break;
        case BPP_SRC_16: val = 16; break;
        case BPP_SRC_24: val = 24; break;
        case BPP_SRC_32: val = 32; break;
        default: val = 0; break;
        }
        break;
    case QEMU_PY_FB_COLOR_ORDER:
        val = get_color_order(rdata);
        break;
    case QEMU_PY_FB_BYTE_ORDER:
        val = get_byte_order(rdata);
        break;
    case QEMU_PY_FB_PIXEL_ORDER:
        val = get_pixel_order(rdata);
        break;
    case QEMU_PY_FB_ROW_PITCH:
        val = get_row_pitch(rdata);
        break;
    default:
        val = -1;
    }
    return PyLong_FromUnsignedLong(val);
}

static int
qemu_py_render_setattr(qemu_py_render *self, PyObject *obval, void *closure)
{
    /* FIXME: should be target_phys_addr_t?  */
    uint32_t val;
    render_data *rdata = self->rdata;

    if (obval == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete attribute");
        return -1;
    }

    val = PyInt_AsUnsignedLongMask(obval);
    if (PyErr_Occurred())
        return -1;

    switch ((size_t)closure) {
    case QEMU_PY_FB_BASE:
        set_fb_base_from_target(rdata, val);
        break;
    case QEMU_PY_FB_WIDTH:
        set_cols(rdata, val);
        break;
    case QEMU_PY_FB_HEIGHT:
        set_rows(rdata, val);
        break;
    case QEMU_PY_FB_ORIENTATION:
        set_orientation(rdata, val);
        break;
    case QEMU_PY_FB_BLANK:
        set_blank_mode(rdata, val);
        break;
    case QEMU_PY_FB_BPP:
        switch (val) {
        case 1: val = BPP_SRC_1; break;
        case 2: val = BPP_SRC_2; break;
        case 4: val = BPP_SRC_4; break;
        case 8: val = BPP_SRC_8; break;
        case 15: val = BPP_SRC_15; break;
        case 16: val = BPP_SRC_16; break;
        case 24: val = BPP_SRC_24; break;
        case 32: val = BPP_SRC_32; break;
        default: val = get_src_bpp(rdata); break;
        }
        set_src_bpp(rdata, val);
        break;
    case QEMU_PY_FB_COLOR_ORDER:
        set_color_order(rdata, val);
        break;
    case QEMU_PY_FB_BYTE_ORDER:
        set_byte_order(rdata, val);
        break;
    case QEMU_PY_FB_PIXEL_ORDER:
        set_pixel_order(rdata, val);
        break;
    case QEMU_PY_FB_ROW_PITCH:
        set_row_pitch(rdata, val);
        break;
    default:
        val = -1;
        break;
    }
    return 0;
}

static PyGetSetDef qemu_py_render_getseters [] = {
    {"base", (getter)qemu_py_render_getattr,
             (setter)qemu_py_render_setattr,
     "base address", (void *)(size_t)QEMU_PY_FB_BASE},
    {"width", (getter)qemu_py_render_getattr,
              (setter)qemu_py_render_setattr,
     "screen width", (void *)(size_t)QEMU_PY_FB_WIDTH},
    {"height", (getter)qemu_py_render_getattr,
              (setter)qemu_py_render_setattr,
     "screen height", (void *)(size_t)QEMU_PY_FB_HEIGHT},
    {"orientation", (getter)qemu_py_render_getattr,
              (setter)qemu_py_render_setattr,
     "screen orientation", (void *)(size_t)QEMU_PY_FB_ORIENTATION},
    {"blank", (getter)qemu_py_render_getattr,
              (setter)qemu_py_render_setattr,
     "blanking mode", (void *)(size_t)QEMU_PY_FB_BLANK},
    {"bpp", (getter)qemu_py_render_getattr,
            (setter)qemu_py_render_setattr,
     "color depth", (void *)(size_t)QEMU_PY_FB_BPP},
    {"color_order", (getter)qemu_py_render_getattr,
                    (setter)qemu_py_render_setattr,
     "color order", (void *)(size_t)QEMU_PY_FB_COLOR_ORDER},
    {"byte_order", (getter)qemu_py_render_getattr,
                   (setter)qemu_py_render_setattr,
     "byte order", (void *)(size_t)QEMU_PY_FB_BYTE_ORDER},
    {"pixel_order", (getter)qemu_py_render_getattr,
                    (setter)qemu_py_render_setattr,
     "pixel packing order", (void *)(size_t)QEMU_PY_FB_PIXEL_ORDER},
    {"row_pitch", (getter)qemu_py_render_getattr,
                  (setter)qemu_py_render_setattr,
     "row pitch", (void *)(size_t)QEMU_PY_FB_ROW_PITCH},
    {NULL}  /* Sentinel */

};

static PyMethodDef qemu_py_render_methods[] = {
    {"get", (PyCFunction)qemu_py_render_get, METH_VARARGS|METH_KEYWORDS,
     "Restore state"},
    {"put", (PyCFunction)qemu_py_render_put, METH_VARARGS|METH_KEYWORDS,
     "Save state"},
    {NULL}  /* Sentinel */
};

static PyTypeObject qemu_py_renderType = {
    PyObject_HEAD_INIT(NULL)
    0,                                    /* ob_size */
    "qemu.render",                        /* tp_name */
    sizeof(qemu_py_render),               /* tp_basicsize */
    0,                                    /* tp_itemsize */
    (destructor)qemu_py_render_dealloc,   /* tp_dealloc */
    0,                                    /* tp_print */
    0,                                    /* tp_getattr */
    0,                                    /* tp_setattr */
    0,                                    /* tp_compare */
    0,                                    /* tp_repr */
    0,                                    /* tp_as_number */
    0,                                    /* tp_as_sequence */
    0,                                    /* tp_as_mapping */
    0,                                    /* tp_hash  */
    0,                                    /* tp_call */
    0,                                    /* tp_str */
    0,                                    /* tp_getattro */
    0,                                    /* tp_setattro */
    0,                                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                   /* tp_flags */
    "QEMU Framebuffer Render Engine",     /* tp_doc */
    0,                                    /* tp_traverse */
    0,                                    /* tp_clear */
    0,		                          /* tp_richcompare */
    0,		                          /* tp_weaklistoffset */
    0,		                          /* tp_iter */
    0,		                          /* tp_iternext */
    qemu_py_render_methods,               /* tp_methods */
    qemu_py_render_members,               /* tp_members */
    qemu_py_render_getseters,             /* tp_getset */
    0,                                    /* tp_base */
    0,                                    /* tp_dict */
    0,                                    /* tp_descr_get */
    0,                                    /* tp_descr_set */
    0,                                    /* tp_dictoffset */
    (initproc)qemu_py_render_init,        /* tp_init */
    0,                                    /* tp_alloc */
    0,                                    /* tp_new */
};


typedef struct {
    PyObject_HEAD
    CharDriverState *chr;
    PyObject *can_receive_cb;
    PyObject *receive_cb;
    PyObject *event_cb;
} qemu_py_chardev;

static void qemu_py_chardev_dealloc(qemu_py_chardev *self)
{
    Py_CLEAR(self->can_receive_cb);
    Py_CLEAR(self->receive_cb);
    Py_CLEAR(self->event_cb);
    self->ob_type->tp_free((PyObject*)self);
}

static int qemu_py_chardev_can_receive(void *opaque)
{
    qemu_py_chardev *self = opaque;
    PyObject *obval;
    int val;

    if (!self->can_receive_cb)
        return 0;

    obval = PyObject_CallFunctionObjArgs(self->can_receive_cb, NULL);
    qemu_py_assert(obval);
    val = PyInt_AsLong(obval);
    qemu_py_assert(!PyErr_Occurred());
    Py_DECREF(obval);
    return val;
}

static void qemu_py_chardev_receive(void *opaque, const uint8_t *buf, int size)
{
    qemu_py_chardev *self = opaque;
    PyObject *obval;
    PyObject *obbuf;

    if (!self->receive_cb)
        return;

    obbuf = PyBuffer_FromMemory((void *)buf, size);
    qemu_py_assert(obbuf);
    obval = PyObject_CallFunctionObjArgs(self->receive_cb, obbuf, NULL);
    Py_DECREF(obbuf);
    qemu_py_assert(obval);
    Py_DECREF(obval);
}

static void qemu_py_chardev_event(void *opaque, int event)
{
    qemu_py_chardev *self = opaque;
    PyObject *obval;
    PyObject *obevent;

    if (!self->event_cb)
        return;

    obevent = PyInt_FromLong(event);
    qemu_py_assert(obevent);
    obval = PyObject_CallFunctionObjArgs(self->event_cb, obevent, NULL);
    Py_DECREF(obevent);
    qemu_py_assert(obval);
    Py_DECREF(obval);
}

static int qemu_py_chardev_init(qemu_py_chardev *self, PyObject *args,
                                PyObject *kwds)
{
    static char *kwlist[] = {"chardev", NULL};
    PyObject *obchr;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &obchr))
        return -1; 

    self->chr = PyCObject_AsVoidPtr(obchr);
    if (PyErr_Occurred())
        return -1;

    if (self->chr) {
        qemu_chr_add_handlers(self->chr, qemu_py_chardev_can_receive,
                              qemu_py_chardev_receive,
                              qemu_py_chardev_event, self);
    }

    return 0;
}

static PyObject *qemu_py_chardev_handle_connect(qemu_py_chardev *self,
                                     PyObject *args)
{
    if (!self->chr)
        Py_RETURN_NONE;
    
    qemu_chr_connect(self->chr);
    
    Py_RETURN_NONE; 
}

static PyObject *qemu_py_chardev_set_handlers(qemu_py_chardev *self,
                                              PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"can_receive", "receive", "event", NULL};
    PyObject *obcan_receive;
    PyObject *obreceive;
    PyObject *obevent = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|O", kwlist,
                                     &obcan_receive, &obreceive, &obevent))
        return NULL;

    if (!self->chr)
        Py_RETURN_NONE;

    Py_CLEAR(self->can_receive_cb);
    Py_CLEAR(self->receive_cb);
    Py_CLEAR(self->event_cb);
    if (!PyCallable_Check(obcan_receive)) {
        PyErr_SetString(PyExc_TypeError,
                        "can_receive handler must be callable");
        return NULL;
    }
    self->can_receive_cb = obcan_receive;
    Py_INCREF(obcan_receive);
    if (!PyCallable_Check(obreceive)) {
        PyErr_SetString(PyExc_TypeError, "receive handler must be callable");
        return NULL;
    }
    self->receive_cb = obreceive;
    Py_INCREF(obreceive);
    if (obevent != Py_None) {
    if (!PyCallable_Check(obreceive)) {
        PyErr_SetString(PyExc_TypeError, "event handler must be callable");
        return NULL;
    }
        self->event_cb = obevent;
        Py_INCREF(obevent);
    }
    Py_RETURN_NONE;
}

static PyObject *qemu_py_chardev_write(qemu_py_chardev *self,
                                       PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"data", NULL};
    PyObject *obdata;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &obdata))
        return NULL;

    if (!self->chr)
        Py_RETURN_NONE;

    if (PyString_Check(obdata)) {
        char *data;
        Py_ssize_t len;
        if (PyString_AsStringAndSize(obdata, &data, &len) < 0)
            return NULL;
        qemu_chr_write(self->chr, (uint8_t *)data, len);
    } else {
        uint8_t ch;
        ch = PyInt_AsLong(obdata);
        if (PyErr_Occurred())
            return NULL;
        qemu_chr_write(self->chr, &ch, 1);
    }
    Py_RETURN_NONE;
}

static PyMemberDef qemu_py_chardev_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef qemu_py_chardev_methods[] = {
    {"handle_connect", (PyCFunction)qemu_py_chardev_handle_connect,
      METH_NOARGS,
      "Handle character device connect if required"},
    {"set_handlers", (PyCFunction)qemu_py_chardev_set_handlers,
      METH_VARARGS|METH_KEYWORDS,
     "Set event handlers"},
    {"write", (PyCFunction)qemu_py_chardev_write, METH_VARARGS|METH_KEYWORDS,
     "Write a byte or string"},
    {NULL}  /* Sentinel */
};

static PyTypeObject qemu_py_chardevType = {
    PyObject_HEAD_INIT(NULL)
    0,                                    /* ob_size */
    "qemu.chardev",                       /* tp_name */
    sizeof(qemu_py_chardev),              /* tp_basicsize */
    0,                                    /* tp_itemsize */
    (destructor)qemu_py_chardev_dealloc,  /* tp_dealloc */
    0,                                    /* tp_print */
    0,                                    /* tp_getattr */
    0,                                    /* tp_setattr */
    0,                                    /* tp_compare */
    0,                                    /* tp_repr */
    0,                                    /* tp_as_number */
    0,                                    /* tp_as_sequence */
    0,                                    /* tp_as_mapping */
    0,                                    /* tp_hash  */
    0,                                    /* tp_call */
    0,                                    /* tp_str */
    0,                                    /* tp_getattro */
    0,                                    /* tp_setattro */
    0,                                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                   /* tp_flags */
    "QEMU Character Device",              /* tp_doc */
    0,                                    /* tp_traverse */
    0,                                    /* tp_clear */
    0,		                          /* tp_richcompare */
    0,		                          /* tp_weaklistoffset */
    0,		                          /* tp_iter */
    0,		                          /* tp_iternext */
    qemu_py_chardev_methods,              /* tp_methods */
    qemu_py_chardev_members,              /* tp_members */
    0,                                    /* tp_getset */
    0,                                    /* tp_base */
    0,                                    /* tp_dict */
    0,                                    /* tp_descr_get */
    0,                                    /* tp_descr_set */
    0,                                    /* tp_dictoffset */
    (initproc)qemu_py_chardev_init,       /* tp_init */
    0,                                    /* tp_alloc */
    0,                                    /* tp_new */
};


typedef struct {
    PyObject_HEAD
    QEMUDevice *qdev;
} qemu_py_devclass;

static void qemu_py_devclass_dealloc(qemu_py_devclass *self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static int qemu_py_devclass_init(qemu_py_devclass *self, PyObject *args,
                                 PyObject *kwds)
{
    static char *kwlist[] = {"qdev", NULL};
    PyObject *obqdev;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &obqdev))
        return -1; 

    self->qdev = PyCObject_AsVoidPtr(obqdev);
    if (PyErr_Occurred())
        return -1;

    return 0;
}


/* FIXME: Turn ths into an actual IRQ object?  */
static PyObject *qemu_py_set_irq_level(qemu_py_devclass *self, PyObject *args)
{
    int ok;
    int irq_num;
    int level;

    ok = PyArg_ParseTuple(args, "ii", &irq_num, &level);
    if (!ok)
      return NULL;

    if (!self->qdev) {
        PyErr_SetString(PyExc_ValueError, "device not initialized");
        return NULL;
    }
    qdev_set_irq_level(self->qdev, irq_num, level);

    Py_RETURN_NONE;
}

/* PyLong_AsUnsignedLongLongMask doesn't seem to work on Int objects.
   This wrapper automatically picks the right routine.  */
static target_phys_addr_t qemu_py_physaddr_from_pynum(PyObject *ob)
{
    if (PyInt_Check(ob))
        return PyInt_AsUnsignedLongMask(ob);
    return PyLong_AsUnsignedLongLongMask(ob);
}

static PyObject *qemu_py_dma_readb(qemu_py_chardev *self, PyObject *args,
                                   PyObject *kwds)
{
    static char *kwlist[] = {"addr", NULL};
    PyObject *obaddr;
    target_phys_addr_t addr;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &obaddr))
        return NULL;

    addr = qemu_py_physaddr_from_pynum(obaddr);
    if (PyErr_Occurred())
        return NULL;

    return PyInt_FromLong(ldub_phys(addr));
}

static PyObject *qemu_py_dma_writeb(qemu_py_chardev *self, PyObject *args,
                                    PyObject *kwds)
{
    static char *kwlist[] = {"addr", "value", NULL};
    PyObject *obaddr;
    PyObject *obval;
    target_phys_addr_t addr;
    uint8_t val;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist,
                                     &obaddr, &obval))
        return NULL;

    addr = qemu_py_physaddr_from_pynum(obaddr);
    if (PyErr_Occurred())
        return NULL;

    val = PyInt_AsLong(obval);
    if (PyErr_Occurred())
        return NULL;

    stb_phys(addr, val);

    Py_RETURN_NONE;
}

static PyObject *qemu_py_dma_readl(qemu_py_chardev *self, PyObject *args,
                                   PyObject *kwds)
{
    static char *kwlist[] = {"addr", NULL};
    PyObject *obaddr;
    target_phys_addr_t addr;
    uint8_t buf[4];
    uint32_t val;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &obaddr))
        return NULL;

    addr = qemu_py_physaddr_from_pynum(obaddr);
    if (PyErr_Occurred())
        return NULL;

    if (addr & 3) {
        cpu_physical_memory_read(addr, buf, 4);
        val = ldl_p(buf);
    } else {
        val = ldl_phys(addr);
    }
    return PyLong_FromUnsignedLong(val);
}

static PyObject *qemu_py_dma_writel(qemu_py_chardev *self, PyObject *args,
                                    PyObject *kwds)
{
    static char *kwlist[] = {"addr", "value", NULL};
    PyObject *obaddr;
    PyObject *obval;
    target_phys_addr_t addr;
    uint8_t buf[4];
    uint32_t val;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist,
                                     &obaddr, &obval))
        return NULL;

    addr = qemu_py_physaddr_from_pynum(obaddr);
    if (PyErr_Occurred())
        return NULL;

    val = PyInt_AsLong(obval);
    if (PyErr_Occurred())
        return NULL;

    if (addr & 3) {
        stl_p(buf, val);
        cpu_physical_memory_write(addr, buf, 4);
    } else {
        stl_phys(addr, val);
    }

    Py_RETURN_NONE;
}

static void qemu_py_set_irq_input(void *opaque, int irq, int level)
{
    PyObject *fn = opaque;
    PyObject *obirq;
    PyObject *oblevel;
    PyObject *obval;

    obirq = PyInt_FromLong(irq);
    qemu_py_assert(obirq);
    oblevel = level ? Py_True : Py_False;
    Py_INCREF(oblevel);
    obval = PyObject_CallFunctionObjArgs(fn, obirq, oblevel, NULL);
    qemu_py_assert(obval);
    Py_DECREF(obval);
    Py_DECREF(obirq);
    Py_DECREF(oblevel);
}

static PyObject *qemu_py_create_interrupts(qemu_py_devclass *self,
                                           PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"callback", "count", NULL};
    PyObject *obcallback;
    int count;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oi", kwlist,
                                     &obcallback, &count))
        return NULL;

    if (!PyCallable_Check(obcallback)) {
        PyErr_SetString(PyExc_TypeError, "irq handler must be callable");
        return NULL;
    }

    Py_INCREF(obcallback);
    qdev_create_interrupts(self->qdev, qemu_py_set_irq_input, obcallback,
                           count);

    Py_RETURN_NONE;
}

static PyObject *qemu_py_dummy_loadsave(qemu_py_devclass *self, PyObject *args)
{
    Py_RETURN_NONE;
}

static PyMemberDef qemu_py_devclass_members[] = {
#if 0
    {"irqs", T_INT, offsetof(qemu_py_devclass, num_irqs), 0,
     "Number of irqs"},
#endif
    {NULL}  /* Sentinel */
};

static PyMethodDef qemu_py_devclass_methods[] = {
    {"set_irq_level", (PyCFunction)qemu_py_set_irq_level, METH_VARARGS,
     "Set IRQ state"},
    {"create_interrupts", (PyCFunction)qemu_py_create_interrupts,
     METH_VARARGS|METH_KEYWORDS,
     "Create IRQ inputs"},
    {"dma_readb", (PyCFunction)qemu_py_dma_readb, METH_VARARGS|METH_KEYWORDS,
     "Read a byte from system memory"},
    {"dma_writeb", (PyCFunction)qemu_py_dma_writeb, METH_VARARGS|METH_KEYWORDS,
     "Write a byte to system memory"},
    {"dma_readl", (PyCFunction)qemu_py_dma_readl, METH_VARARGS|METH_KEYWORDS,
     "Read a 32-bit word from system memory"},
    {"dma_writel", (PyCFunction)qemu_py_dma_writel, METH_VARARGS|METH_KEYWORDS,
     "Write a 32-bit word to system memory"},
    {"load", (PyCFunction)qemu_py_dummy_loadsave, METH_VARARGS,
     "load snapshot state"},
    {"save", (PyCFunction)qemu_py_dummy_loadsave, METH_VARARGS,
     "save snapshot state"},
    {NULL}  /* Sentinel */
};

static PyTypeObject qemu_py_devclassType = {
    PyObject_HEAD_INIT(NULL)
    0,                                    /* ob_size */
    "qemu.devclass",                      /* tp_name */
    sizeof(qemu_py_devclass),             /* tp_basicsize */
    0,                                    /* tp_itemsize */
    (destructor)qemu_py_devclass_dealloc, /* tp_dealloc */
    0,                                    /* tp_print */
    0,                                    /* tp_getattr */
    0,                                    /* tp_setattr */
    0,                                    /* tp_compare */
    0,                                    /* tp_repr */
    0,                                    /* tp_as_number */
    0,                                    /* tp_as_sequence */
    0,                                    /* tp_as_mapping */
    0,                                    /* tp_hash  */
    0,                                    /* tp_call */
    0,                                    /* tp_str */
    0,                                    /* tp_getattro */
    0,                                    /* tp_setattro */
    0,                                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,                   /* tp_flags */
    "QEMU device class objects",          /* tp_doc */
    0,                                    /* tp_traverse */
    0,                                    /* tp_clear */
    0,		                          /* tp_richcompare */
    0,		                          /* tp_weaklistoffset */
    0,		                          /* tp_iter */
    0,		                          /* tp_iternext */
    qemu_py_devclass_methods,             /* tp_methods */
    qemu_py_devclass_members,             /* tp_members */
    0,                                    /* tp_getset */
    0,                                    /* tp_base */
    0,                                    /* tp_dict */
    0,                                    /* tp_descr_get */
    0,                                    /* tp_descr_set */
    0,                                    /* tp_dictoffset */
    (initproc)qemu_py_devclass_init,      /* tp_init */
    0,                                    /* tp_alloc */
    0,                                    /* tp_new */
};

typedef struct
{
    PyObject *dev;
    int n;
    PyObject *region;
} qemu_py_callback_info;

static uint32_t qemu_py_read(void *opaque, target_phys_addr_t offset)
{
    qemu_py_callback_info *info = opaque;
    PyObject *r;
    PyObject *fn;
    PyObject *oboffset;
    PyObject *obval;
    uint32_t val;

    r = info->region;
    qemu_py_assert(r);
    fn = PyObject_GetAttrString(r, "readl");
    qemu_py_assert(fn);
    oboffset = PyInt_FromLong(offset);
    obval = PyObject_CallFunctionObjArgs(fn, info->dev, oboffset, NULL);
    Py_DECREF(oboffset);
    Py_DECREF(fn);
    qemu_py_assert(obval);
    val = PyLong_AsUnsignedLongMask(obval);
    qemu_py_assert(!PyErr_Occurred());
    Py_DECREF(obval);
    return val;
}

static void qemu_py_write(void *opaque, target_phys_addr_t offset,
                                uint32_t value)
{
    qemu_py_callback_info *info = opaque;
    PyObject *r;
    PyObject *fn;
    PyObject *oboffset;
    PyObject *obval;

    r = info->region;
    qemu_py_assert(r);
    fn = PyObject_GetAttrString(r, "writel");
    qemu_py_assert(fn);
    oboffset = PyInt_FromLong(offset);
    obval = PyLong_FromUnsignedLong(value);
    PyObject_CallFunctionObjArgs(fn, info->dev, oboffset, obval, NULL);
    Py_DECREF(oboffset);
    Py_DECREF(obval);
    Py_DECREF(fn);
    qemu_py_assert(!PyErr_Occurred());
}

static CPUReadMemoryFunc *qemu_py_readfn[] = {
     qemu_py_read,
     qemu_py_read,
     qemu_py_read
};

static CPUWriteMemoryFunc *qemu_py_writefn[] = {
     qemu_py_write,
     qemu_py_write,
     qemu_py_write
};

static void qemu_py_dev_create(QEMUDevice *dev)
{
    PyObject *ob;
    PyObject *devclass;
    PyObject *regions;
    PyObject *obqdev;
    qemu_py_devclass *self;
    qemu_py_callback_info *callbacks;
    Py_ssize_t pos;
    PyObject *properties;
    PyObject *key;
    PyObject *value;
    int num_regions;
    int i;

    devclass = qdev_get_class_opaque(dev);
    obqdev = PyCObject_FromVoidPtr(dev, NULL);
    ob = PyObject_CallFunctionObjArgs(devclass, obqdev, NULL);
    qemu_py_assert(ob);
    Py_DECREF(obqdev);
    self = (qemu_py_devclass *)ob;
    self->qdev = dev;
    qdev_set_opaque(dev, ob);

    value = PyString_FromString(qdev_get_name(dev));
    PyObject_SetAttrString(ob, "name", value);

    regions = PyObject_GetAttrString(devclass, "regions");
    qemu_py_assert(regions);
    num_regions = PyList_Size(regions);
    callbacks = qemu_mallocz(num_regions * sizeof(callbacks[0]));
    for (i = 0; i < num_regions; i++) {
        callbacks[i].dev = ob;
        callbacks[i].n = i;
        callbacks[i].region = PyList_GetItem(regions, i);
        Py_INCREF(callbacks[i].region);
        qdev_set_region_opaque(dev, i, &callbacks[i]);
    }
    Py_DECREF(regions);

    properties = PyObject_GetAttrString(devclass, "properties");
    qemu_py_assert(properties);
    pos = 0;
    while (PyDict_Next(properties, &pos, &key, &value)) {
        char *key_name = PyString_AsString(key);
        qemu_py_assert(key_name);
        if (strcmp(key_name, "chardev") == 0) {
            CharDriverState *chr;
            PyObject *obchr;
            chr = qdev_get_chardev(dev);
            obchr = PyCObject_FromVoidPtr(chr, NULL);
            qemu_py_assert(obchr);
            value =
              PyObject_CallFunctionObjArgs((PyObject *)&qemu_py_chardevType,
                                           obchr, NULL);
            qemu_py_assert(value);
            Py_DECREF(obchr);
        } else if (PyLong_Check(value) || PyInt_Check(value)) {
            long intval;
            intval = qdev_get_property_int(dev, key_name);
            value = PyInt_FromLong(intval);
        } else {
            const char *strval;
            strval = qdev_get_property_string(dev, key_name);
            value = PyString_FromString(strval);
        }
        if (PyDict_SetItem(properties, key, value) < 0) {
            Py_DECREF(value);
        }

    }
    Py_DECREF(properties);

    PyObject_CallMethod(ob, "create", NULL);
    qemu_py_assert(!PyErr_Occurred());
}

static PyObject *qemu_py_create_file(QEMUFile *f)
{
    PyObject *obfile;
    PyObject *obarg;

    obarg = PyCObject_FromVoidPtr(f, NULL);
    if (!obarg)
        return NULL;
    obfile = PyObject_CallFunctionObjArgs((PyObject *)&qemu_py_fileType,
                                          obarg, NULL);
    Py_DECREF(obarg);
    return obfile;
}

static void qemu_py_save(QEMUFile *f, void *opaque)
{
    PyObject *self = opaque;
    PyObject *obfile;
    PyObject *obresult;


    obfile = qemu_py_create_file(f);
    qemu_py_assert(obfile);
    obresult = PyObject_CallMethod(self, "save", "O", obfile);
    qemu_py_assert(obresult);
    Py_DECREF(obfile);
    Py_DECREF(obresult);
}

static int qemu_py_load(QEMUFile *f, void *opaque, int version_id)
{
    PyObject *self = opaque;
    PyObject *obfile;
    PyObject *obresult;

    if (version_id != 1)
        return -EINVAL;

    obfile = qemu_py_create_file(f);
    qemu_py_assert(obfile);
    obresult = PyObject_CallMethod(self, "load", "O", obfile);
    /* TODO: Graceful error handling.  */
    qemu_py_assert(obresult);
    Py_DECREF(obfile);
    Py_DECREF(obresult);

    return 0;
}

static PyObject *qemu_py_register_device(PyObject *self, PyObject *args)
{
    int ok;
    QEMUDeviceClass *dc;
    PyObject *devclass;
    PyObject *regions;
    PyObject *attr;
    PyObject *properties;
    Py_ssize_t pos;
    PyObject *key;
    PyObject *value;
    int i;
    char *name;
    int num_irqs;

    ok = PyArg_ParseTuple(args, "O", &devclass);
    if (!ok)
        return NULL;

    if (!PyType_Check(devclass)
        || !PyObject_IsSubclass(devclass, (PyObject *)&qemu_py_devclassType)) {

        PyErr_SetString(PyExc_TypeError, "Expected qemu.devclass derivative");
        return NULL;
    }

    Py_INCREF(devclass);

    attr = PyObject_GetAttrString(devclass, "irqs");
    if (!attr)
        return NULL;
    num_irqs = PyInt_AsLong(attr);
    if (PyErr_Occurred())
        return NULL;
    Py_DECREF(attr);

    attr = PyObject_GetAttrString(devclass, "name");
    if (!attr)
        return NULL;
    name = PyString_AsString(attr);
    if (PyErr_Occurred())
        return NULL;

    dc = qdev_new(name, qemu_py_dev_create, num_irqs);

    Py_DECREF(attr);

    qdev_add_class_opaque(dc, devclass);

    regions = PyObject_GetAttrString(devclass, "regions");
    if (!regions)
        return NULL;
    if (!PyList_Check(regions)) {
        PyErr_SetString(PyExc_TypeError, "devclass.regions must be a list");
        return NULL;
    }
    for (i = 0; i < PyList_Size(regions); i++) {
        target_phys_addr_t size;
        PyObject *r;

        r = PyList_GetItem(regions, i);
        if (!r)
            return NULL;

        attr = PyObject_GetAttrString(r, "size");
        size = PyInt_AsLong(attr);
        if (PyErr_Occurred())
            return NULL;
        qdev_add_registers(dc, qemu_py_readfn, qemu_py_writefn, size);
        Py_DECREF(attr);
    }
    Py_DECREF(regions);

    properties = PyObject_GetAttrString(devclass, "properties");
    if (!properties)
        return NULL;
    if (!PyDict_Check(properties)) {
        Py_DECREF(properties);
        PyErr_SetString(PyExc_TypeError,
                        "qemu.devclass.properties must be a dict");
        return NULL;
    }
    pos = 0;
    while (PyDict_Next(properties, &pos, &key, &value)) {
        char *key_name = PyString_AsString(key);
        if (!key_name)
            goto bad_property;
        if (strcmp(key_name, "chardev") == 0) {
            qdev_add_chardev(dc);
        } else if (PyString_Check(value)) {
            qdev_add_property_string(dc, key_name, PyString_AsString(value));
        } else if (PyLong_Check(value) || PyInt_Check(value)) {
            long intval;
            intval = PyLong_AsLong(value);
            if (PyErr_Occurred())
                goto bad_property;
            qdev_add_property_int(dc, key_name, PyLong_AsLong(value));
        } else {
            PyErr_SetString(PyExc_TypeError,
                "qemu.devclass.properties value must be int or string");
        bad_property:
            Py_DECREF(properties);
            return NULL;
        }
    }
    Py_DECREF(properties);

    /* TODO: Implement savevm versioning.  */
    qdev_add_savevm(dc, 1, qemu_py_save, qemu_py_load);

    Py_RETURN_NONE;
}

static PyObject *qemu_py_get_clock(PyObject *self, PyObject *args)
{
    return PyLong_FromUnsignedLongLong((uint64_t)qemu_get_clock(vm_clock));
}

static PyObject *qemu_py_start_time(PyObject *self, PyObject *args)
{
    struct tm tm;
    uint64_t t;
    qemu_get_timedate(&tm, 0);
    t = mktimegm(&tm);
    return PyLong_FromUnsignedLongLong(t);
}

static void qemu_py_keyboard_event(void *opaque, int keycode)
{
    PyObject *fn = opaque;
    PyObject *obval;
    PyObject *obkey;

    obkey = PyInt_FromLong(keycode);
    obval = PyObject_CallFunctionObjArgs(fn, obkey, NULL);
    qemu_py_assert(obval);
    Py_DECREF(obkey);
    Py_DECREF(obval);
}

static PyObject *qemu_py_register_keyboard(PyObject *self, PyObject *args,
                                           PyObject *kwds)
{
    static char *kwlist[] = {"handler", NULL};
    PyObject *fn;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &fn))
        return NULL; 

    if (!PyCallable_Check(fn)) {
        PyErr_SetString(PyExc_TypeError, "keyboard handler must be callable");
        return NULL;
    }

    Py_INCREF(fn);
    gui_register_dev_key_callback(qemu_py_keyboard_event, fn);

    Py_RETURN_NONE;
}

static void qemu_py_mouse_event(void *opaque, int dx, int dy, int dz,
                                int buttons)
{
    PyObject *fn = opaque;
    PyObject *obval;
    PyObject *obx;
    PyObject *oby;
    PyObject *obz;
    PyObject *obbuttons;

    obx = PyInt_FromLong(dx);
    oby = PyInt_FromLong(dy);
    obz = PyInt_FromLong(dz);
    obbuttons = PyInt_FromLong(buttons);
    obval = PyObject_CallFunctionObjArgs(fn, obx, oby, obz, obbuttons, NULL);
    qemu_py_assert(obval);
    Py_DECREF(obx);
    Py_DECREF(oby);
    Py_DECREF(obz);
    Py_DECREF(obbuttons);
    Py_DECREF(obval);
}

static PyObject *qemu_py_register_mouse(PyObject *self, PyObject *args,
                                        PyObject *kwds)
{
    static char *kwlist[] = {"handler", "absolute", "devid", NULL};
    PyObject *fn;
    int absolute;
	char *devid;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Ois", kwlist, &fn, &absolute, &devid))
        return NULL; 

    if (!PyCallable_Check(fn)) {
        PyErr_SetString(PyExc_TypeError, "mouse handler must be callable");
        return NULL;
    }

    Py_INCREF(fn);
    gui_register_mouse_event_handler(qemu_py_mouse_event, fn, absolute, devid);

    Py_RETURN_NONE;
}

static PyMethodDef qemu_py_methods[] =
{
    {"register_device", qemu_py_register_device, METH_VARARGS,
     "Register new device class"},
    {"get_clock", qemu_py_get_clock, METH_NOARGS,
     "Get current virtual time"},
    {"start_time", qemu_py_start_time, METH_NOARGS,
     "Get VM start time (seconds sice epoch)"},
    {"register_keyboard", (PyCFunction)qemu_py_register_keyboard,
     METH_VARARGS|METH_KEYWORDS,
     "Register keyboard event handler"},
    {"register_mouse", (PyCFunction)qemu_py_register_mouse,
     METH_VARARGS|METH_KEYWORDS,
     "Register mouse event handler"},
    {NULL}
};

static void qemu_py_add_class(PyObject *module, const char *name,
                              PyTypeObject *type)
{
    if (!type->tp_new)
        type->tp_new = PyType_GenericNew;
    if (PyType_Ready(type) < 0)
        qemu_py_die();
    Py_INCREF(type);
    PyModule_AddObject(module, name, (PyObject *)type);
}

static void qemu_py_init_interface(void)
{
    PyObject *m;
    m = Py_InitModule("qemu", qemu_py_methods);

    /* FIXME: Add default attributes to qemu_py_devclass.  */
    qemu_py_add_class(m, "devclass", &qemu_py_devclassType);
    qemu_py_add_class(m, "ioregion", &qemu_py_ioregionType);
    qemu_py_add_class(m, "chardev", &qemu_py_chardevType);
    qemu_py_add_class(m, "render", &qemu_py_renderType);
    qemu_py_add_class(m, "palette", &qemu_py_paletteType);
    qemu_py_add_class(m, "ptimer", &qemu_py_ptimerType);
    qemu_py_add_class(m, "file", &qemu_py_fileType);
    qemu_py_add_class(m, "memregion", &qemu_py_memRegionType);
}

#define PLUGIN_INIT_SCRIPT "import sys\nsys.path.insert(0, \"%s/plugins\")"
void qemu_python_init(char *argv0)
{
    char *buf;
    Py_SetProgramName(argv0);
    Py_Initialize();

    /* Nasty hack to look for plugin modules.  */
    buf = qemu_mallocz(strlen(bios_dir) + strlen(PLUGIN_INIT_SCRIPT));
    sprintf(buf, PLUGIN_INIT_SCRIPT, bios_dir);
#ifdef _WIN32
    {
        char *p;
        /* Windows path separators (backslash) are interpreted as escape
           characters in a python string.  Fortunately python also accepts
           unix path separators (forward slash), so use those instead.  */
        for (p = buf; *p; p++) {
            if (*p == '\\')
                *p = '/';
        }
    }
#endif
    PyRun_SimpleString(buf);
    qemu_free(buf);

    qemu_py_init_interface();

    qemu_py_load_module("qemu_arm_plugins");
}
