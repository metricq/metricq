def to_timestamp(something):
    if isinstance(something, float):
        return int(something * 1e9)
    # TODO other stuff
    raise TypeError("Unknown timestamp thingy {} ({})".format(something, type(something)))
