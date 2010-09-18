
from apport.hookutils import *

def add_info(report):
    report['GconfSettingsOfGMP'] = command_output(['gconftool-2', '-R', '/apps/gnome-media-player'])

