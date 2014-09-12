/**
 * @author Manuel Guenther <manuel.guenther@idiap.ch>
 * @date Wed Jul  2 14:38:18 CEST 2014
 *
 * @brief Binds the DCTFeatures class to python
 *
 * Copyright (C) 2011-2014 Idiap Research Institute, Martigny, Switzerland
 */

#include "main.h"
#include <boost/format.hpp>

/******************************************************************/
/************ Constructor Section *********************************/
/******************************************************************/

static auto FeatureExtractor_doc = bob::extension::ClassDoc(
  BOB_EXT_MODULE_PREFIX ".FeatureExtractor",
  "A bounding box class storing top, left, height and width of an rectangle",
  0
).add_constructor(
  bob::extension::FunctionDoc(
    "__init__",
    "Constructs a new Bounding box from the given top-left position and the size of the rectangle",
    0,
    true
  )
  .add_prototype("patch_size", "")
  .add_prototype("patch_size, extractors", "")
  .add_prototype("patch_size, template, [overlap], [square], [min_size], [max_size]", "")
  .add_prototype("other", "")
  .add_prototype("hdf5", "")
  .add_parameter("patch_size", "(int, int)", "The size of the patch to extract from the images")
  .add_parameter("extractors", "[:py:class:`bob.ip.base.LBP`]", "The LBP classes to use as extractors")
  .add_parameter("template", ":py:class:`bob.ip.base.LBP`", "The LBP classes to use as template for all extractors")
  .add_parameter("overlap", "bool", "[default: False] Should overlapping LBPs be created?")
  .add_parameter("square", "bool", "[default: False] Should only square LBPs be created?")
  .add_parameter("min_size", "int", "[default: 1] The minimum radius of LBP")
  .add_parameter("max_size", "int", "[default: MAX_INT] The maximum radius of LBP (limited by patch size)")
  .add_parameter("other", ":py:class:`FeatureExtractor`", "The feature extractor to use for copy-construction")
  .add_parameter("hdf5", ":py:class:`bob.ip.base.HDF5File`", "The HDF5 file to read the extractors from")
);


static int FeatureExtractor_init(FeatureExtractorObject* self, PyObject* args, PyObject* kwargs) {
  TRY

  char* kwlist1[] = {c("patch_size"), NULL};
  char* kwlist2[] = {c("patch_size"), c("extractors"), NULL};
  char* kwlist3[] = {c("patch_size"), c("template"), c("overlap"), c("square"), c("min_size"), c("max_size"), NULL};
//  char* kwlist4[] = {c("other"), NULL};
//  char* kwlist5[] = {c("hdf5"), NULL};

  // get the number of command line arguments
  Py_ssize_t nargs = (args?PyTuple_Size(args):0) + (kwargs?PyDict_Size(kwargs):0);

  blitz::TinyVector<int,2> patch_size;
  if (nargs == 1){
    // get first arguments
    PyObject* first = PyTuple_Size(args) ? PyTuple_GET_ITEM(args, 0) : PyList_GET_ITEM(make_safe(PyDict_Values(kwargs)).get(), 0);
    if (PyBobIoHDF5File_Check(first)){
      self->cxx.reset(new FeatureExtractor(*((PyBobIoHDF5FileObject*)first)->f));
      return 0;
    }
    if (FeatureExtractor_Check(first)){
      self->cxx.reset(new FeatureExtractor(*((FeatureExtractorObject*)first)->cxx));
      return 0;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "(ii)", kwlist1, &patch_size[0], &patch_size[1])){
      return -1;
    }
    self->cxx.reset(new FeatureExtractor(patch_size));
    return 0;
  }

  // more than one arg
  PyObject* list;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "(ii)O!", kwlist2, &patch_size[0], &patch_size[1], &PyList_Type, &list)){
    PyErr_Clear();
    // try the third option
    PyBobIpBaseLBPObject* lbp;
    PyObject* overlap = 0,* square = 0;
    int min_size = 1, max_size = std::numeric_limits<int>::max();

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "(ii)O&|O!O!ii", kwlist3, &patch_size[0], &patch_size[1], &PyBobIpBaseLBP_Converter, &lbp, &PyBool_Type, &overlap, &PyBool_Type, &square, &min_size, &max_size)) return -1;
    auto lbp_ = make_safe(lbp);
    self->cxx.reset(new FeatureExtractor(patch_size, *lbp->cxx, f(overlap), f(square), min_size, max_size));
    return 0;
  }
  // with list of LBP's
  std::vector<boost::shared_ptr<bob::ip::base::LBP>> lbps(PyList_GET_SIZE(list));
  for (Py_ssize_t i = 0; i < PyList_GET_SIZE(list); ++i){
    PyObject* lbp = PyList_GET_ITEM(list, i);
    if (!PyBobIpBaseLBP_Check(lbp)){
      PyErr_Format(PyExc_TypeError, "%s : expected a list of LBP objects, but object number %d is of type `%s'", Py_TYPE(self)->tp_name, (int)i, Py_TYPE(lbp)->tp_name);
      return -1;
    }
    lbps[i] = ((PyBobIpBaseLBPObject*)lbp)->cxx;
  }
  self->cxx.reset(new FeatureExtractor(patch_size, lbps));
  return 0;
  CATCH("cannot create FeatureExtractor", -1)
}

static void FeatureExtractor_delete(FeatureExtractorObject* self) {
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int FeatureExtractor_Check(PyObject* o) {
  return PyObject_IsInstance(o, reinterpret_cast<PyObject*>(&FeatureExtractor_Type));
}

#if 0 // TODO:
static PyObject* FeatureExtractor_RichCompare(FeatureExtractorObject* self, PyObject* other, int op) {
  TRY
  if (!FeatureExtractor_Check(other)) {
    PyErr_Format(PyExc_TypeError, "cannot compare `%s' with `%s'", Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return 0;
  }
  auto other_ = reinterpret_cast<FeatureExtractorObject*>(other);
  switch (op) {
    case Py_EQ:
      if (*self->cxx==*other_->cxx) Py_RETURN_TRUE; else Py_RETURN_FALSE;
    case Py_NE:
      if (*self->cxx==*other_->cxx) Py_RETURN_FALSE; else Py_RETURN_TRUE;
    default:
      Py_INCREF(Py_NotImplemented);
      return Py_NotImplemented;
  }
  CATCH("cannot compare FeatureExtractor objects", 0)
}

PyObject* FeatureExtractor_Str(FeatureExtractorObject* self) {
  TRY
  return PyString_FromString((boost::format("<BB topleft=(%3.2d,%3.2d), bottomright=(%3.2d,%3.2d)>") % self->cxx->top() % self->cxx->left() % self->cxx->bottom() % self->cxx->right()).str().c_str());
  CATCH("cannot create __repr__ string", 0)
}
#endif


/******************************************************************/
/************ Variables Section ***********************************/
/******************************************************************/

static auto image = bob::extension::VariableDoc(
  "image",
  "array_like <2D, uint8>",
  "The (prepared) image the next features will be extracted from, read access only"
);
PyObject* FeatureExtractor_image(FeatureExtractorObject* self, void*){
  TRY
  return PyBlitzArrayCxx_AsConstNumpy(self->cxx->getImage());
  CATCH("image could not be read", 0)
}

static auto model_indices = bob::extension::VariableDoc(
  "model_indices",
  "array_like <1D, int32>",
  "The indices at which the features are extracted, read and write access"
);
PyObject* FeatureExtractor_get_model_indices(FeatureExtractorObject* self, void*){
  TRY
  return PyBlitzArrayCxx_AsConstNumpy(self->cxx->getModelIndices());
  CATCH("model_indices could not be read", 0)
}
int FeatureExtractor_set_model_indices(FeatureExtractorObject* self, PyObject* value, void*){
  TRY
  PyBlitzArrayObject* data;
  if (!PyBlitzArray_Converter(value, &data)) return 0;
  if (data->type_num != NPY_INT32 || data->ndim != 1){
    PyErr_Format(PyExc_TypeError, "model_indices can only be 1D and of type int32");
    return -1;
  }
  self->cxx->setModelIndices(*PyBlitzArrayCxx_AsBlitz<int32_t, 1>(data));
  return 0;
  CATCH("model_indices could not be set", -1)
}

static auto number_of_features = bob::extension::VariableDoc(
  "number_of_features",
  "int",
  "The length of the feature vector that will be extracted by this class, read access only"
);
PyObject* FeatureExtractor_number_of_features(FeatureExtractorObject* self, void*){
  TRY
  return Py_BuildValue("i", self->cxx->numberOfFeatures());
  CATCH("number_of_features could not be read", 0)
}

static auto number_of_labels = bob::extension::VariableDoc(
  "number_of_labels",
  "int",
  "The maximum label for the features in this class, read access only"
);
PyObject* FeatureExtractor_number_of_labels(FeatureExtractorObject* self, void*){
  TRY
  return Py_BuildValue("i", self->cxx->getMaxLabel());
  CATCH("number_of_labels could not be read", 0)
}

static auto extractors = bob::extension::VariableDoc(
  "extractors",
  "[:py:class:`bob.ip.LBP`]",
  "The LBP extractors, read access only"
);
PyObject* FeatureExtractor_extractors(FeatureExtractorObject* self, void*){
  TRY
  auto lbps = self->cxx->getExtractors();
  PyObject* list = PyList_New(lbps.size());
  auto list_ = make_safe(list);
  for (Py_ssize_t i = 0; i < PyList_GET_SIZE(list); ++i){
    PyBobIpBaseLBPObject* lbp = reinterpret_cast<PyBobIpBaseLBPObject*>(PyBobIpBaseLBP_Type.tp_alloc(&PyBobIpBaseLBP_Type, 0));
    lbp->cxx = lbps[i];
    PyList_SET_ITEM(list, i, Py_BuildValue("O", lbp));
  }
  Py_INCREF(list);
  return list;
  CATCH("extractors could not be read", 0)
}

static auto patch_size = bob::extension::VariableDoc(
  "patch_size",
  "(int, int)",
  "The expected size of the patch that this extractor can handle, read access only"
);
PyObject* FeatureExtractor_patch_size(FeatureExtractorObject* self, void*){
  TRY
  return Py_BuildValue("ii", self->cxx->patchSize()[0], self->cxx->patchSize()[1]);
  CATCH("patch_size could not be read", 0)
}


static PyGetSetDef FeatureExtractor_getseters[] = {
    {
      image.name(),
      (getter)FeatureExtractor_image,
      0,
      image.doc(),
      0
    },
    {
      model_indices.name(),
      (getter)FeatureExtractor_get_model_indices,
      (setter)FeatureExtractor_set_model_indices,
      model_indices.doc(),
      0
    },
    {
      number_of_features.name(),
      (getter)FeatureExtractor_number_of_features,
      0,
      number_of_features.doc(),
      0
    },
    {
      number_of_labels.name(),
      (getter)FeatureExtractor_number_of_labels,
      0,
      number_of_labels.doc(),
      0
    },
    {
      extractors.name(),
      (getter)FeatureExtractor_extractors,
      0,
      extractors.doc(),
      0
    },
    {
      patch_size.name(),
      (getter)FeatureExtractor_patch_size,
      0,
      patch_size.doc(),
      0
    },
    {0}  /* Sentinel */
};

/******************************************************************/
/************ Functions Section ***********************************/
/******************************************************************/

static auto append = bob::extension::FunctionDoc(
  "append",
  "Appends the given feature extractor or LBP class to this one",
  "With this function you can either append a complete feature extractor, or a partial axtractor (i.e., a single LBP class) including the offset positions for them",
  true
)
.add_prototype("other")
.add_prototype("lbp, offsets")
.add_parameter("other", ":py:class:`FeatureExtractor`", "All LBP classes and offset positions of the given extractor will be appended")
.add_parameter("lbp", ":py:class:`bob.ip.base.LBP`", "The LBP extractor that will be added")
.add_parameter("offsets", "[(int,int)]", "The offset positions at which the given LBP will be extracted")
;

static PyObject* FeatureExtractor_append(FeatureExtractorObject* self, PyObject* args, PyObject* kwargs) {
  TRY
  static char* kwlist1[] = {c("other"), 0};
  static char* kwlist2[] = {c("lbp"), c("offsets"), 0};

  // get the number of command line arguments
  Py_ssize_t nargs = (args?PyTuple_Size(args):0) + (kwargs?PyDict_Size(kwargs):0);

  if (nargs == 1){
    FeatureExtractorObject* other;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist1, &FeatureExtractor_Type, &other)) return 0;
    self->cxx->append(*other->cxx);
  } else {
    PyBobIpBaseLBPObject* lbp;
    PyObject* list;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&O!", kwlist2, &PyBobIpBaseLBP_Converter, &lbp, &PyList_Type, &list)) return 0;
    auto lbp_ = make_safe(lbp);
    // extract list
    std::vector<blitz::TinyVector<int32_t, 2>> offsets(PyList_GET_SIZE(list));
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(list); ++i){
      if (!PyArg_ParseTuple(PyList_GET_ITEM(list,i), "hh", &offsets[i][0], &offsets[i][1])){
        PyErr_Format(PyExc_TypeError, "%s : expected a list of (int, int) tuples, but object number %d not", Py_TYPE(self)->tp_name, (int)i);
        return 0;
      }
    }
    self->cxx->append(lbp->cxx, offsets);
  }
  Py_RETURN_NONE;
  CATCH("cannot append", 0)
}

static auto extractor = bob::extension::FunctionDoc(
  "extractor",
  "Get the LBP feature extractor associated with the given feature index",
  0,
  true
)
.add_prototype("index", "lbp")
.add_parameter("index", "int", "The feature index for which the extractor should be retrieved")
.add_return("lbp", ":py:class:`bob.ip.base.LBP`", "The feature extractor for the given feature index")
;
static PyObject* FeatureExtractor_extractor(FeatureExtractorObject* self, PyObject* args, PyObject* kwargs) {
  TRY
  static char* kwlist[] = {c("index"), 0};

  int index;
  // by shape
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &index)) return 0;
  PyBobIpBaseLBPObject* lbp = reinterpret_cast<PyBobIpBaseLBPObject*>(PyBobIpBaseLBP_Type.tp_alloc(&PyBobIpBaseLBP_Type, 0));
  lbp->cxx = self->cxx->extractor(index);
  return Py_BuildValue("N", lbp);
  CATCH("cannot get extractor", 0)
}

static auto offset = bob::extension::FunctionDoc(
  "offset",
  "Get the offset position associated with the given feature index",
  0,
  true
)
.add_prototype("index", "offset")
.add_parameter("index", "int", "The feature index for which the extractor should be retrieved")
.add_return("offset", "(int,int)", "The offset position for the given feature index")
;
static PyObject* FeatureExtractor_offset(FeatureExtractorObject* self, PyObject* args, PyObject* kwargs) {
  TRY
  static char* kwlist[] = {c("index"), 0};

  int index;
  // by shape
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &index)) return 0;
  auto offset = self->cxx->offset(index);
  return Py_BuildValue("ii", offset[0], offset[1]);
  CATCH("cannot get offset", 0)
}

static auto prepare = bob::extension::FunctionDoc(
  "prepare",
  "Take the given image to perform the next extraction steps for the given scale",
  "If ``compute_integral_square_image`` is enabled, the (internally stored) integral square image is computed as well. "
  "This image is required to compute the variance of the pixels in a given patch, see :py:func:`mean_variance`",
  true
)
.add_prototype("image, scale, [compute_integral_square_image]")
.add_parameter("image", "array_like <2D, uint8 or float>", "The image that should be used in the next extraction step")
.add_parameter("scale", "float", "The scale of the image to extract")
.add_parameter("compute_integral_square_image", "bool", "[Default: ``False``] : Enable the computation of the integral square image")
;
static PyObject* FeatureExtractor_prepare(FeatureExtractorObject* self, PyObject* args, PyObject* kwargs) {
  TRY
  static char* kwlist[] = {c("image"), c("scale"), c("compute_integral_square_image"), 0};

  PyBlitzArrayObject* image;
  double scale;
  PyObject* cisi = 0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&d|O!", kwlist, &PyBlitzArray_Converter, &image, &scale, &PyBool_Type, &cisi)){
    return 0;
  }
  auto image_ = make_safe(image);
  if (image->ndim != 2){
    PyErr_Format(PyExc_TypeError, "%s : The input image must be 2D, not %dD", Py_TYPE(self)->tp_name, (int)image->ndim);
    return 0;
  }
  switch (image->type_num){
    case NPY_UINT8: self->cxx->prepare(*PyBlitzArrayCxx_AsBlitz<uint8_t,2>(image), scale, f(cisi)); Py_RETURN_NONE;
    case NPY_FLOAT64: self->cxx->prepare(*PyBlitzArrayCxx_AsBlitz<double,2>(image), scale, f(cisi)); Py_RETURN_NONE;
    default:
      PyErr_Format(PyExc_TypeError, "%s : The input image must be of type uint8 or float", Py_TYPE(self)->tp_name);
      return 0;
  }
  CATCH("cannot prepare image", 0)
}

static auto extract_all = bob::extension::FunctionDoc(
  "extract_all",
  "Extracts all features into the given dataset of (training) features at the given index",
  "This function exists to extract training features for several training patches. "
  "To avoid data copying, the full training dataset, and the current training feature index need to be provided.",
  true
)
.add_prototype("bounding_box, dataset, dataset_index")
.add_parameter("bounding_box", ":py:class:`BoundingBox`", "The bounding box for which the features should be extracted")
.add_parameter("dataset", "array_like <2D, uint16>", "The (training) dataset, into which the features should be extracted; must be of shape (#training_patches, :py:attr:`number_of_features`)")
.add_parameter("dataset_index", "int", "The index of the current training patch")
;
static PyObject* FeatureExtractor_extract_all(FeatureExtractorObject* self, PyObject* args, PyObject* kwargs) {
  TRY
  static char* kwlist[] = {c("bounding_box"), c("dataset"), c("dataset_index"), 0};

  BoundingBoxObject* bb;
  PyBlitzArrayObject* dataset;
  int index;
  // by shape
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O&i", kwlist, &BoundingBox_Type, &bb, &PyBlitzArray_OutputConverter, &dataset, &index)){
    return 0;
  }
  auto dataset_ = make_safe(dataset);
  auto ds = PyBlitzArrayCxx_AsBlitz<uint16_t, 2>(dataset, "dataset");
  if (!ds) return 0;
  self->cxx->extractAll(*bb->cxx, *ds, index);
  Py_RETURN_NONE;
  CATCH("cannot extract all features", 0)
}

static auto extract_indexed = bob::extension::FunctionDoc(
  "extract_indexed",
  "Extracts the features only at the required locations, which defaults to :py:attr:`model_indices`",
  0,
  true
)
.add_prototype("bounding_box, feature_vector, [indices]", "sim")
.add_parameter("bounding_box", ":py:class:`BoundingBox`", "The bounding box for which the features should be extracted")
.add_parameter("feature_vector", "array_like <1D, uint16>", "The feature vector, into which the features should be extracted; must be of size :py:attr:`number_of_features`")
.add_parameter("indices", "array_like<1D,int32>", "The indices, for which the features should be extracted; if not given, :py:attr:`model_indices` is used (must be set beforehands)")
;
static PyObject* FeatureExtractor_extract_indexed(FeatureExtractorObject* self, PyObject* args, PyObject* kwargs) {
  TRY
  static char* kwlist[] = {c("bounding_box"), c("feature_vector"), c("indices"), 0};

  BoundingBoxObject* bb;
  PyBlitzArrayObject* fv, *indices = 0;
  // by shape
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O&|O&", kwlist, &BoundingBox_Type, &bb, &PyBlitzArray_OutputConverter, &fv, &PyBlitzArray_Converter, &indices)){
    return 0;
  }
  auto fv_ = make_safe(fv);
  auto indices_ = make_xsafe(indices);
  auto f = PyBlitzArrayCxx_AsBlitz<uint16_t, 1>(fv, "feature_vector");
  if (!f) return 0;

  if (indices){
    auto i = PyBlitzArrayCxx_AsBlitz<int32_t, 1>(indices, "indices");
    if (!i) return 0;
    self->cxx->extractIndexed(*bb->cxx, *f, *i);
  } else {
    self->cxx->extractSome(*bb->cxx, *f);
  }
  Py_RETURN_NONE;
  CATCH("cannot extract indexed features", 0)
}

static auto mean_variance = bob::extension::FunctionDoc(
  "mean_variance",
  "Computes the mean (and the variance) of the pixel gray values in the given bounding box",
  0,
  true
)
.add_prototype("bounding_box, [compute_variance]", "mv")
.add_parameter("bounding_box", ":py:class:`BoundingBox`", "The bounding box for which the mean (and variance) shoulf be calculated")
.add_parameter("compute_variance", "bool", "[Default: ``False``] If enabled, the variance is computed as well; requires the ``compute_integral_square_image`` enabled in the :py:func:`prepare` function")
.add_return("mv", "float or (float, float)", "The mean (or the mean and the variance) of the pixel gray values for the given bounding box")
;
static PyObject* FeatureExtractor_mean_variance(FeatureExtractorObject* self, PyObject* args, PyObject* kwargs) {
  TRY
  static char* kwlist[] = {c("bounding_box"), c("compute_variance"), c("indices"), 0};

  BoundingBoxObject* bb;
  PyObject* cv = 0;
  // by shape
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!", kwlist, &BoundingBox_Type, &bb, &PyBool_Type, &cv)){
    return 0;
  }

  if (f(cv)){
    auto res = self->cxx->meanAndVariance(*bb->cxx);
    return Py_BuildValue("dd", res[0], res[1]);
  } else {
    double res = self->cxx->mean(*bb->cxx);
    return Py_BuildValue("d", res);
  }
  CATCH("cannot compute mean (and variance)", 0)
}

static auto load = bob::extension::FunctionDoc(
  "load",
  "Loads the extractors from the given HDF5 file",
  0,
  true
)
.add_prototype("hdf5")
.add_parameter("hdf5", ":py:class:`bob.ip.base.HDF5File`", "The file to read from")
;
static PyObject* FeatureExtractor_load(FeatureExtractorObject* self, PyObject* args, PyObject* kwargs) {
  TRY
  static char* kwlist[] = {c("hdf5"), 0};

  PyBobIoHDF5FileObject* hdf5;
  // by shape
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&", kwlist, &PyBobIoHDF5File_Converter, &hdf5)) return 0;

  self->cxx->load(*hdf5->f);
  Py_RETURN_NONE;
  CATCH("cannot load", 0)
}

static auto save = bob::extension::FunctionDoc(
  "save",
  "Saves the extractors to the given HDF5 file",
  0,
  true
)
.add_prototype("hdf5")
.add_parameter("hdf5", ":py:class:`bob.ip.base.HDF5File`", "The file to write to")
;
static PyObject* FeatureExtractor_save(FeatureExtractorObject* self, PyObject* args, PyObject* kwargs) {
  TRY
  static char* kwlist[] = {c("hdf5"), 0};

  PyBobIoHDF5FileObject* hdf5;
  // by shape
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&", kwlist, &PyBobIoHDF5File_Converter, &hdf5)) return 0;

  self->cxx->save(*hdf5->f);
  Py_RETURN_NONE;
  CATCH("cannot save", 0)
}


static PyMethodDef FeatureExtractor_methods[] = {
  {
    append.name(),
    (PyCFunction)FeatureExtractor_append,
    METH_VARARGS|METH_KEYWORDS,
    append.doc()
  },
  {
    extractor.name(),
    (PyCFunction)FeatureExtractor_extractor,
    METH_VARARGS|METH_KEYWORDS,
    extractor.doc()
  },
  {
    offset.name(),
    (PyCFunction)FeatureExtractor_offset,
    METH_VARARGS|METH_KEYWORDS,
    offset.doc()
  },
  {
    prepare.name(),
    (PyCFunction)FeatureExtractor_prepare,
    METH_VARARGS|METH_KEYWORDS,
    prepare.doc()
  },
  {
    extract_all.name(),
    (PyCFunction)FeatureExtractor_extract_all,
    METH_VARARGS|METH_KEYWORDS,
    extract_all.doc()
  },
  {
    extract_indexed.name(),
    (PyCFunction)FeatureExtractor_extract_indexed,
    METH_VARARGS|METH_KEYWORDS,
    extract_indexed.doc()
  },
  {
    mean_variance.name(),
    (PyCFunction)FeatureExtractor_mean_variance,
    METH_VARARGS|METH_KEYWORDS,
    mean_variance.doc()
  },
  {
    load.name(),
    (PyCFunction)FeatureExtractor_load,
    METH_VARARGS|METH_KEYWORDS,
    load.doc()
  },
  {
    save.name(),
    (PyCFunction)FeatureExtractor_save,
    METH_VARARGS|METH_KEYWORDS,
    save.doc()
  },
  {0} /* Sentinel */
};

/******************************************************************/
/************ Module Section **************************************/
/******************************************************************/

// Define the DCTFeatures type struct; will be initialized later
PyTypeObject FeatureExtractor_Type = {
  PyVarObject_HEAD_INIT(0,0)
  0
};

bool init_FeatureExtractor(PyObject* module)
{
  // initialize the type struct
  FeatureExtractor_Type.tp_name = FeatureExtractor_doc.name();
  FeatureExtractor_Type.tp_basicsize = sizeof(FeatureExtractorObject);
  FeatureExtractor_Type.tp_flags = Py_TPFLAGS_DEFAULT;
  FeatureExtractor_Type.tp_doc = FeatureExtractor_doc.doc();
//  FeatureExtractor_Type.tp_repr = (reprfunc)FeatureExtractor_Str;
//  FeatureExtractor_Type.tp_str = (reprfunc)FeatureExtractor_Str;

  // set the functions
  FeatureExtractor_Type.tp_new = PyType_GenericNew;
  FeatureExtractor_Type.tp_init = reinterpret_cast<initproc>(FeatureExtractor_init);
  FeatureExtractor_Type.tp_dealloc = reinterpret_cast<destructor>(FeatureExtractor_delete);
//  FeatureExtractor_Type.tp_richcompare = reinterpret_cast<richcmpfunc>(FeatureExtractor_RichCompare);
  FeatureExtractor_Type.tp_methods = FeatureExtractor_methods;
  FeatureExtractor_Type.tp_getset = FeatureExtractor_getseters;

  // check that everything is fine
  if (PyType_Ready(&FeatureExtractor_Type) < 0)
    return false;

  // add the type to the module
  Py_INCREF(&FeatureExtractor_Type);
  return PyModule_AddObject(module, "FeatureExtractor", (PyObject*)&FeatureExtractor_Type) >= 0;
}
