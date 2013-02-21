
all: roundtrip

mus2pmx:
	../mus2pmx ex1.mus > ex1-output.pmx

pmx2mus:
	../pmx2mus ex1.pmx ex1-output.mus

roundtrip: mus2pmx pmx2mus
	../mus2pmx ex1-output.mus > ex1-roundtrip.pmx
	@echo Round-trip difference:
	diff ex1.pmx ex1-roundtrip.pmx

# If you have https://github.com/craigsapp/prettypmx :
ex1-pretty:
	../mus2pmx ex1.mus | prettypmx > ex1-pretty.pmx

clean:
	-rm ex1-pretty.pmx
	-rm ex1-output.pmx
	-rm ex1-output.mus
	-rm ex1-roundtrip.pmx
