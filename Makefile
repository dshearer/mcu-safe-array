.PHONY: doc serve check

check:
	python test/run.py

doc:
	doxygen doxygen.conf

serve: doc
	serve doc/html