
CXX=g++ -m64
CXXFLAGS=-I../common -Iobjs/ -O3 -std=c++11 -Wall -fPIC

APP_NAME=mandelbrot
OBJDIR=objs
COMMONDIR=../common

PPM_CXX=$(COMMONDIR)/ppm.cpp
PPM_OBJ=$(addprefix $(OBJDIR)/, $(subst $(COMMONDIR)/,, $(PPM_CXX:.cpp=.o)))

ANALYSIS_FILE=analysis.txt


default: $(APP_NAME)

.PHONY: dirs clean

dirs:
		/bin/mkdir -p $(OBJDIR)/

analysis: $(APP_NAME)
	@if [ -f $(ANALYSIS_FILE) ]; then \
		echo "analysis.txt already exists."; \
	else \
		threads=1; while [ $$threads -le 16 ]; do \
			./$(APP_NAME) -t $$threads >> $(ANALYSIS_FILE); \
			echo "Running Mandelbrot with $$threads threads..."; \
			threads=$$((threads + 1)); \
		done; \
		sed -Eni 's/.*([0-9]+\.[0-9]+)x.*from ([0-9]+).*/\2 \1/p' analysis.txt; \
		echo "analysis.txt has been generated."; \
	fi
	@gnuplot -e "set terminal pngcairo size 800,600; set output 'output'; set yrange [0:*]; set xlabel 'threads'; set ylabel 'speedup'; plot '$(ANALYSIS_FILE)' with lines" 
	@display output

multirun:
	@threads=1; while [ $$threads -le 50 ]; do \
		./$(APP_NAME) -t 8 >> $(ANALYSIS_FILE); \
		threads=$$((threads + 1)); \
	done
	# sed -Eni 's/.*([0-9]+\.[0-9]+)x.*from ([0-9]+).*/\2 \1/p' analysis.txt
	# @gnuplot -e "set terminal pngcairo size 800,600; set output 'output'; set yrange [0:*]; set xlabel 'threads'; set ylabel 'speedup'; plot temp.txt with lines"
	# @display output


clean:
		/bin/rm -rf $(OBJDIR) *.ppm *~ $(APP_NAME) .cache compile_commands.json analysis.txt output

OBJS=$(OBJDIR)/main.o $(OBJDIR)/mandelbrotSerial.o $(OBJDIR)/mandelbrotThread.o $(PPM_OBJ)

$(APP_NAME): dirs $(OBJS)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJS) -lm -lpthread

$(OBJDIR)/%.o: %.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/%.o: $(COMMONDIR)/%.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/main.o: $(COMMONDIR)/CycleTimer.h

