artifacts := ../artifacts
out       := $(artifacts)/preview
res       := 128
views     := top,front,left,front_top_left
render    := ./render.py
fov	      := 30
bg := "\#000000"
fg := "\#ffffff"

objs   := $(wildcard $(artifacts)/*.obj)
stamps := $(patsubst $(artifacts)/%.obj,$(out)/%.done,$(objs))

.PHONY: all clean list

all: $(stamps)

list:
	@printf '%s\n' $(notdir $(basename $(objs)))

$(out)/%.done: $(artifacts)/%.obj $(render)
	@mkdir -p $(out)
	$(render) $< -o $(out) -r $(res) --views $(views) --background "$(bg)" --pigment "$(fg)"
	@touch $@

clean:
	rm -rf $(out)