import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))


def P(name):
    """Full path of a data file, of the lifted model or of a result file, given its file name."""
    if name.startswith('clauses-bloc1') or name == 'voisinage1':
        d = os.path.join(ROOT, 'data', 'flood_block')
    elif name == 'lifted_model.json':
        d = os.path.join(ROOT, 'model')
    else:
        d = os.path.join(ROOT, 'results')
    os.makedirs(d, exist_ok=True)
    return os.path.join(d, name)


def OUT(name):
    """Full path of a generated figure or table."""
    d = os.path.join(ROOT, 'figures', 'output')
    os.makedirs(d, exist_ok=True)
    return os.path.join(d, name)
