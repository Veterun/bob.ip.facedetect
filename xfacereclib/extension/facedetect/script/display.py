
import argparse
import facereclib
import bob
import numpy
import math
import xbob.boosting
import os

from .. import utils
from ..detector import Sampler, MBLBPFeatures, load_features

def command_line_options(command_line_arguments):

  parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.ArgumentDefaultsHelpFormatter)

  parser.add_argument('--test-image', '-i', required=True, help = "Select the image to detect the face in.")
  parser.add_argument('--distance', '-s', type=int, default=5, help = "The distance with which the image should be scanned.")
  parser.add_argument('--scale-base', '-S', type=float, default = math.pow(2.,-1./16.), help = "The logarithmic distance between two scales (should be between 0 and 1).")
  parser.add_argument('--first-scale', '-f', type=float, default = 0.5, help = "The first scale of the image to consider (should be between 0 and 1, higher values will slow down the detection process).")
  parser.add_argument('--trained-file', '-r', default = 'detector.hdf5', help = "The file to write the resulting trained detector into.")
  parser.add_argument('--prediction-threshold', '-t', type = float, help = "If given, all detection above this threshold will be displayed.")
  parser.add_argument('--prune-detections', '-p', type=float, help = "If given, detections that overlap with the given threshold are pruned")

  facereclib.utils.add_logger_command_line_option(parser)
  args = parser.parse_args(command_line_arguments)
  facereclib.utils.set_verbosity_level(args.verbose)

  return args

#def classify(classifier, features):
#  return classifier(features)

def main(command_line_arguments = None):
  args = command_line_options(command_line_arguments)

  facereclib.utils.debug("Loading strong classifier from file %s" % args.trained_file)
  # load classifier and feature extractor
  f = bob.io.HDF5File(args.trained_file)
  f.cd("/Machine")
  classifier = xbob.boosting.BoostedMachine(f)
#  classifier = xbob.boosting.core.boosting.BoostMachine(hdf5file=f)
  f.cd("/Features")
  feature_extractor = load_features(f)
  feature_extractor.set_model(classifier)

  sampler = Sampler(distance=args.distance, scale_factor=args.scale_base, first_scale=args.first_scale)

  # load test file
  test_image = bob.io.load(args.test_image)
  if test_image.ndim == 3:
    test_image = bob.ip.rgb_to_gray(test_image)

  detections = []
  # get the detection scores for the image
  for bounding_box, features in sampler.iterate(test_image, feature_extractor):
    prediction = classifier(features)
    if args.prediction_threshold is None or prediction > args.prediction_threshold:
      detections.append((prediction, bounding_box))
      facereclib.utils.debug("Found bounding box %s with value %f" % (str(bounding_box), prediction))

  # prune detections
  detections = utils.prune(detections, args.prune_detections)


  facereclib.utils.info("Number of (pruned) detections: %d" % len(detections))
  highest_detection = detections[0][0]
  facereclib.utils.info("Best detection with value %f at %s: " % (highest_detection, str(detections[0][1])))

  if not args.prediction_threshold:
    detections = detections[:1]

  test_image = bob.io.load(args.test_image)
  for detection in detections:
    detection[1].draw(test_image, (int(255. * detection[0] / highest_detection),0,0))

  bob.io.save(test_image, 'result.png')

