# SPDX-FileCopyrightText: 2019 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

# Download all pull requests as patches that match a specific label
# Usage: python download-patches-by-label.py <Label to Match> <Root Path Folder to DL to>

import requests, sys, json, urllib3.request, shutil, subprocess, os, traceback

tagline = sys.argv[2]

http = urllib3.PoolManager()
dl_list = {}

def check_individual(labels):
    for label in labels:
        if (label["name"] == sys.argv[1]):
            return True
    return False

def do_page(page):
    url = 'https://api.github.com/repos/yuzu-emu/yuzu/pulls?page=%s' % page
    response = requests.get(url)
    if (response.ok):
        j = json.loads(response.content)
        if j == []:
            return
        for pr in j:
            if (check_individual(pr["labels"])):
                pn = pr["number"]
                print("Matched PR# %s" % pn)
                print(subprocess.check_output(["git", "fetch", "https://github.com/yuzu-emu/yuzu.git", "pull/%s/head:pr-%s" % (pn, pn), "-f", "--no-recurse-submodules"]))
                print(subprocess.check_output(["git", "merge", "--squash", "pr-%s" % pn]))
                print(subprocess.check_output(["git", "commit", "-m\"Merge %s PR %s\"" % (tagline, pn)]))

try:
    for i in range(1,30):
        do_page(i)
except:
    traceback.print_exc(file=sys.stdout)
    sys.exit(-1)
