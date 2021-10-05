from flask import Flask, request, jsonify
from werkzeug.utils import secure_filename
import os
import subprocess
import json

def run_starc_api(arguments):
    binary_path = os.path.abspath(os.path.dirname(__file__)) + '/bin/'
    starc_api_command = 'LD_LIBRARY_PATH=' + binary_path + ' ' + binary_path + 'starcapi'
    output = subprocess.Popen(starc_api_command + ' ' + arguments,
                              shell=True, stderr=subprocess.PIPE).stderr.read()
    return json.loads(output)

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = '/tmp/'

base_route = '/api/v1/'

@app.route(base_route + '/stories/', methods=['POST'])
def stories():
    # check if the post request has the file part
    if 'document' not in request.files:
        return 'No document part', 400
    file = request.files['document']
    # If the user does not select a file, the browser submits an
    # empty file without a filename.
    if file.filename == '':
        return 'No selected document', 400
    if file:
        filename = secure_filename(file.filename)
        file.save(os.path.join(app.config['UPLOAD_FOLDER'], filename))
    # Run starc API to processing saved document
    arguments = 'stories ' + filename
    return jsonify(run_starc_api(arguments))

@app.route(base_route + '/stories/<story>/characters/')
def characters(story):
    arguments = 'characters ' + story
    return jsonify(run_starc_api(arguments))

@app.route(base_route + '/stories/<story>/characters/<character>/')
def character(story, character):
    arguments = 'character ' + story + ' "' + character + '"'
    return jsonify(run_starc_api(arguments))

if __name__ == '__main__':
    app.run()
