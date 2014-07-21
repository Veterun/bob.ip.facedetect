import facereclib

def _all_in(annot, annot_types):
  if annot_types is None: return True
  for t in annot_types:
    if t not in annot: return False
  return True

def _annotations(db, file, annot_types=None):
  # returns the annotations for the given file object
  import xbob.db.detection.utils
  import xbob.db.verification.utils
  try:
    if isinstance(db, xbob.db.detection.utils.Database):
      # detection databases have multiple annotations per file
      annots = db.annotations(file.id)
    elif isinstance(db, xbob.db.verification.utils.Database):
      # verification databases have just one annotation per file
      annots = [db.annotations(file.id)]
    elif isinstance(db, facereclib.databases.DatabaseXBob):
      # verification databases have just one annotation per file
      annots = [db.annotations(file)]
    else:
      raise NotImplementedError("The given database is of no known type.")
  except IOError as e:
    facereclib.utils.warn("Could not get annotations for file %s -- skipping (Error message is: %s)" % (file.path, e))
    return None
  return [a for a in annots if _all_in(a, annot_types)]

def _original_filename(db, file):
  if isinstance(db, facereclib.databases.DatabaseXBob):
    return db.m_database.original_file_name(file)
  else:
    return db.original_file_name(file)


def training_image_annot(databases, limit, annot_types=None):
  facereclib.utils.info("Collecting training data")
  # open database to collect training images
  training_files = []
  for database in databases:
    facereclib.utils.info("Processing database '%s'" % database)
    db = facereclib.utils.resources.load_resource(database, 'database')
    if isinstance(db, facereclib.databases.DatabaseXBob):
      # collect image name and annotations
      training_files.extend([(db.m_database.original_file_name(f), _annotations(db, f, annot_types), f) for f in db.training_files()])
    else:
      # collect image name and annotations
      all_files = db.training_files()
      training_files.extend([(_original_filename(db,all_files[i]), _annotations(db, all_files[i], annot_types), all_files[i]) for i in facereclib.utils.quasi_random_indices(len(all_files), limit)])

  training_files = [training_files[t] for t in facereclib.utils.quasi_random_indices(len(training_files), limit) if training_files[t][1] is not None]

#  for file, annot in training_files:
  for file, annot, _ in training_files:
    facereclib.utils.debug("For training file '%s' loaded annotations '%s'" % (file, str(annot)))

  return training_files


def test_image_annot(databases, protocols, limit, annot_types=None):
  # open database to collect training images
  test_files = []
  for database, protocol in zip(databases, protocols):
    db = facereclib.utils.resources.load_resource(database, 'database')
    if isinstance(db, facereclib.databases.DatabaseXBob):
      # collect image name and annotations
      orig_files = db.test_files()
    else:
      # collect image name and annotations
      orig_files = db.test_files(protocol=protocol)

    orig_files = [orig_files[t] for t in facereclib.utils.quasi_random_indices(len(orig_files), limit)]
    # collect image name and annotations
    test_files.extend([(_original_filename(db, f), _annotations(db, f, annot_types), f) for f in orig_files])

  test_files = [test_files[t] for t in facereclib.utils.quasi_random_indices(len(test_files), limit)]

  for _, annot, file in test_files:
    facereclib.utils.debug("For test file '%s' loaded annotations '%s'" % (file.path, str(annot)))

  return test_files


