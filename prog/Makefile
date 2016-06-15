ola.run: ola.cc
	g++ -w ola.cc -o ola.run -lsndfile

out.wav: ola.run
	./ola.run

.PHONY: run gen
run: ola.run
	./ola.run

gen: out.wav
	cvlc out.wav
