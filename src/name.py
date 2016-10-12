import re

def canonical(s):
    '''
    >>> canonical('/tom')
    '/tom'
    >>> canonical('tom')
    'tom'
    >>> canonical('/tom/.')
    '/tom'
    >>> canonical('/tom/./dick')
    '/tom/dick'
    >>> canonical('/tom/dick/.')
    '/tom/dick'
    >>> canonical('tom/.')
    'tom'
    >>> canonical('tom/./dick')
    'tom/dick'
    >>> canonical('tom/dick/.')
    'tom/dick'
    >>> canonical('/tom/..')
    '/'
    >>> canonical('/tom/../dick')
    '/dick'
    >>> canonical('/tom/dick/..')
    '/tom'
    >>> canonical('tom/..')
    '.'
    >>> canonical('tom/../dick')
    'dick'
    >>> canonical('tom/dick/..')
    'tom'
    >>> canonical('.')
    '.'
    >>> canonical('./tom')
    'tom'
    >>> canonical('/tom/dick/../harry/..')
    '/tom'
    >>> canonical('tom/dick/../harry/..')
    'tom'
    >>> canonical('/tom/dick/../harry/../..')
    '/'
    >>> canonical('tom/dick/../harry/../..')
    '.'
    >>> canonical('tom/dick/../harry')
    'tom/harry'
    >>> canonical('..')
    '..'
    >>> canonical('../tom')
    '../tom'
    >>> canonical('../tom/..')
    '..'
    >>> canonical('./../tom/..')
    '..'
    '''
    rules = ( (re.compile(r'[^./]+/\.\.'), '.')
            , (re.compile(r'/\.$'), '')
            , (re.compile(r'/\.(?P<more>[^.])'), '\g<more>')
            , (re.compile(r'^\./'), '')
            )
    t = None
    while (t is None or t != s):
        t = s[:]
        for (i, (pattern, repl)) in enumerate(rules, start=1):
            s = pattern.sub(repl, t)
            if t != s:
#                print("mapped", repr(t), "to", repr(s), "rule", i)
                break

    if s == "":
        s = "/"
#        print("mapped", repr(""), "to", repr(s), "rule", len(rules))

    return s


if __name__ == "__main__":
    import doctest
    doctest.testmod()
