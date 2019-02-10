import math
import sys
import cv2
import numpy as np
from consts import SPLIT, PX_SAMPLES

if __name__ == '__main__':
	def r():
		def fom(h, v):
			# figure of merit (unapoligetically heuristic) for pixel to be our target LED
			return (h.astype(np.uint32) * (v.astype(np.uint32)) ** 2 / 255)
		def peak_h(h, h0):
			return abs(90 - (h + (180 - h0)) % 180)
		def find(f):
			flat = f.flatten()
			flat_argmax = flat.argpartition(-PX_SAMPLES)[-PX_SAMPLES:] # default flatten is row-major
			return [
				np.mean(flat[flat_argmax]),
				np.average([flat_argmax // f.shape[1], flat_argmax % f.shape[1]], axis=1, weights=flat[flat_argmax]).astype(np.uint16)
			] # y, x
			
		for fname in sys.argv[1:]:
			with open('%s.csv' % fname, 'w') as out:
				out.write('re_confidence,re_y,re_x,im_confidence,im_y,im_x\n')
				V = cv2.VideoCapture(fname)
				codec = cv2.VideoWriter_fourcc(*'mp4v')
				Vout = cv2.VideoWriter('%s_out.mp4' % fname[:-4], codec, V.get(cv2.CAP_PROP_FPS), (1700 * 2, 540))
				argmaxes = []
				i = 0
				while True:
					valid, im = V.read()
					if not valid:
						break
					
					im = im[:540,:1700]
					# f = (f * 255 / np.max(f[:])).astype(np.uint8)
					im_hsv = cv2.cvtColor(im, cv2.COLOR_BGR2HSV)
					# cv2.im_hsvshow('1', 90 - abs(90-(im_hsv[:,:,0])))
					imag = im_hsv[:SPLIT] # crop to avoid reflections and artifacts
					real = im_hsv[SPLIT:]
					imag_f = fom(peak_h(imag[...,0], 0), imag[...,2])
					real_f = fom(peak_h(real[...,0], 60), real[...,2])
					
					argmax_re = find(real_f)
					argmax_im = find(imag_f)
					out.write('%d,%d,%d,' % ((argmax_re[0],) + tuple(argmax_re[1])))
					out.write('%d,%d,%d\n' % ((argmax_im[0],) + tuple(argmax_im[1])))
					sys.stdout.write('\r%s | %d' % (fname, i))
					i += 1
					
					# cv2.polylines(im, [np.array([[0, im_hsv.shape[0] / 4], [im_hsv.shape[1] - 1, im_hsv.shape[0] / 4]])], False, 255)
					real_f_im = np.tile((real_f * 255 / np.max(real_f))[...,np.newaxis], (1, 1, 3)).astype(np.uint8)
					cv2.circle(real_f_im, (int(argmax_re[1][1]), int(argmax_re[1][0])), 10, (0, 255, 0), -1) # real
					imag_f_im = np.tile((imag_f * 255 / np.max(imag_f))[...,np.newaxis], (1, 1, 3)).astype(np.uint8)
					cv2.circle(imag_f_im, (int(argmax_im[1][1]), int(argmax_im[1][0])), 10, (0, 0, 255), -1) # imaginary
					
					frame = np.concatenate((im, np.concatenate((imag_f_im, real_f_im), axis=0)), axis=1)
					# print(np.transpose(frame, (1, 0, 2)).shape)
					Vout.write(frame)
					# cv2.imshow('1', frame)
					# cv2.imshow('1', im) # red filter
					# cv2.waitKey(100)
					
				Vout.release()
				sys.stdout.write('\n')
				sys.stdout.flush()
	r()